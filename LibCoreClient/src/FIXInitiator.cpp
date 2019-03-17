#include "FIXInitiator.h"

#include "CoreClient.h"
#include "Exceptions.h"
#include "OrderBookGlobalAsk.h"
#include "OrderBookGlobalBid.h"
#include "OrderBookLocalAsk.h"
#include "OrderBookLocalBid.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <list>
#include <regex>
#include <thread>

#include <curl/curl.h>

#ifdef _WIN32
#include <crypto/Encryptor.h>
#include <terminal/Common.h>
#else
#include <shift/miscutils/crypto/Encryptor.h>
#include <shift/miscutils/terminal/Common.h>
#endif

using namespace std::chrono_literals;

/* static */ std::string shift::FIXInitiator::s_senderID;
/* static */ std::string shift::FIXInitiator::s_targetID;

// Predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_USERREQUESTTYPE_LOG_ON_USER = FIX::UserRequestType(FIX::UserRequestType_LOG_ON_USER);
static const auto& FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SUBSCRIBE = FIX::SubscriptionRequestType(FIX::SubscriptionRequestType_SNAPSHOT_PLUS_UPDATES);
static const auto& FIXFIELD_SUBSCRIPTIONREQUESTTYPE_UNSUBSCRIBE = FIX::SubscriptionRequestType(FIX::SubscriptionRequestType_DISABLE_PREVIOUS_SNAPSHOT_PLUS_UPDATE_REQUEST);
static const auto& FIXFIELD_MDUPDATETYPE_INCREMENTAL_REFRESH = FIX::MDUpdateType(FIX::MDUpdateType_INCREMENTAL_REFRESH);
static const auto& FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH = FIX::MarketDepth(0);
static const auto& FIXFIELD_MDENTRYTYPE_BID = FIX::MDEntryType(FIX::MDEntryType_BID);
static const auto& FIXFIELD_MDENTRYTYPE_OFFER = FIX::MDEntryType(FIX::MDEntryType_OFFER);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);

/*static*/ inline std::chrono::system_clock::time_point shift::FIXInitiator::s_convertToTimePoint(const FIX::UtcDateOnly& date, const FIX::UtcTimeOnly& time)
{
    return std::chrono::system_clock::from_time_t(date.getTimeT()
        + time.getHour() * 3600
        + time.getMinute() * 60
        + time.getSecond());
}

/**
 * @brief Default constructor for FIXInitiator object.
 */
shift::FIXInitiator::FIXInitiator()
    : m_connected{ false }
    , m_verbose{ false }
    , m_logonSuccess{ false }
    , m_openPricesReady{ false }
    , m_lastTradeTime{ std::chrono::system_clock::time_point() }
{
}

/**
 * @brief Default destructor for FIXInitiator object.
 */
shift::FIXInitiator::~FIXInitiator() // override
{
    for (auto pair_symbol_orderBooks : m_orderBooks)
        for (auto pair_type_orderBook : pair_symbol_orderBooks.second)
            if (pair_type_orderBook.second)
                delete pair_type_orderBook.second;

    {
        std::lock_guard<std::mutex> ucGuard(m_mtxUsernameClientMap);
        m_usernameClientMap.clear();
    }

    disconnectBrokerageCenter();
}

/**
 * @brief Get singleton instance of FIXInitiator
 */
shift::FIXInitiator& shift::FIXInitiator::getInstance()
{
    static FIXInitiator fixInitiator;
    return fixInitiator;
}

void shift::FIXInitiator::connectBrokerageCenter(const std::string& cfgFile, CoreClient* client, const std::string& password, bool verbose, int timeout)
{
    disconnectBrokerageCenter();

    m_username = client->getUsername();

    std::istringstream iss{ password };
    shift::crypto::Encryptor enc;
    iss >> enc >> m_password;

    m_verbose = verbose;

    std::string senderCompID = m_username;
    std::transform(senderCompID.begin(), senderCompID.end(), senderCompID.begin(), ::toupper);

    FIX::SessionSettings settings(cfgFile);
    const FIX::Dictionary& commonDict = settings.get();
    FIX::SessionID sessionID(commonDict.getString("BeginString"),
        FIX::SenderCompID(senderCompID),
        commonDict.getString("TargetCompID"));

    FIX::Dictionary dict;
    settings.set(sessionID, std::move(dict));

    if (commonDict.has("FileLogPath")) {
        m_pLogFactory.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath") + "/" + m_username));
    } else {
        m_pLogFactory.reset(new FIX::ScreenLogFactory(false, false, m_verbose));
    }

    if (commonDict.has("FileStorePath")) {
        m_pMessageStoreFactory.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath") + "/" + m_username));
    } else {
        m_pMessageStoreFactory.reset(new FIX::NullStoreFactory());
    }

    m_pSocketInitiator.reset(new FIX::SocketInitiator(shift::FIXInitiator::getInstance(), *m_pMessageStoreFactory, settings, *m_pLogFactory));
    m_pSocketInitiator->start();

    auto t1 = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lsUniqueLock(m_mtxLogon);
    // Gets notify from fromAdmin() or onLogon()
    m_cvLogon.wait_for(lsUniqueLock, timeout * 1ms);
    auto t2 = std::chrono::steady_clock::now();
    timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    if (m_logonSuccess) {
        std::unique_lock<std::mutex> slUniqueLock(m_mtxStockList);
        // Gets notify when security list is ready
        m_cvStockList.wait_for(slUniqueLock, timeout * 1ms);

        attach(client);
        m_connected = m_logonSuccess.load();
    } else {
        disconnectBrokerageCenter();

        if (timeout <= 0) {
            throw shift::ConnectionTimeout();
        } else {
            throw shift::IncorrectPassword();
        }
    }
}

void shift::FIXInitiator::disconnectBrokerageCenter()
{
    // Stop QuickFIX
    if (m_pSocketInitiator) {
        m_pSocketInitiator->stop();
        m_pSocketInitiator = nullptr;
    }
    m_pMessageStoreFactory = nullptr;
    m_pLogFactory = nullptr;

    // House cleaning:

    // Cancel all pending client requests
    try {
        getMainClient()->cancelAllSamplePricesRequests();
    } catch (...) { // it is OK if there is no main client yet
    } // (we force a disconnect before every connection - even the first one)

    for (auto& client : getAttachedClients()) {
        client->cancelAllSamplePricesRequests();
    }

    // Reset state variables:
    m_connected = false;
    m_verbose = false;
    m_logonSuccess = false;
    m_openPricesReady = false;
    m_lastTradeTime = std::chrono::system_clock::time_point();

    // Stock list is only updated when first connecting,
    // so this allows its update after a disconnection
    {
        std::lock_guard<std::mutex> slGuard(m_mtxStockList);
        m_stockList.clear();
        m_originalName_symbol.clear();
        m_symbol_originalName.clear();
    }
    {
        std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
        m_companyNames.clear();
    }

    // Subscriptions will reset automatically when disconnecting,
    // but we still need to clean up these sets
    {
        std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);
        m_subscribedOrderBookSet.clear();
    }
    {
        std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandleStickSet);
        m_subscribedCandleStickSet.clear();
    }
}

/**
 * @section WARNING
 * Don't call this function directly
 */
void shift::FIXInitiator::attach(shift::CoreClient* client, const std::string& password, int timeout)
{
    // TODO: add auth in BC and add lock after successful logon; use password to auth
    webClientSendUsername(client->getUsername());

    {
        std::lock_guard<std::mutex> ucGuard(m_mtxUsernameClientMap);
        m_usernameClientMap[client->getUsername()] = client;
    }

    client->attach(*this);
}

/**
 * @brief Method used by WebClient to send username
 * (since webclient use one client ID for all users)
 *
 * @param username as the name for the newly login user.
 */
void shift::FIXInitiator::webClientSendUsername(const std::string& username)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserRequest));

    message.setField(FIX::UserRequestID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_USERREQUESTTYPE_LOG_ON_USER); // Required by FIX
    message.setField(FIX::Username(username));

    FIX::Session::sendToTarget(message);
}

std::vector<shift::CoreClient*> shift::FIXInitiator::getAttachedClients()
{
    std::vector<shift::CoreClient*> clientsVector;

    std::lock_guard<std::mutex> ucGuard(m_mtxUsernameClientMap);

    for (auto it = m_usernameClientMap.begin(); it != m_usernameClientMap.end(); ++it) {
        if (it->first != m_username) { // skip MainClient
            clientsVector.push_back(it->second);
        }
    }

    return clientsVector;
}

shift::CoreClient* shift::FIXInitiator::getMainClient()
{
    return getClient(m_username);
}

/**
 * @brief get CoreClient by client name.
 *
 * @param name as a string to provide the client name.
 */
shift::CoreClient* shift::FIXInitiator::getClient(const std::string& name)
{
    std::lock_guard<std::mutex> ucGuard(m_mtxUsernameClientMap);

    if (m_usernameClientMap.find(name) != m_usernameClientMap.end()) {
        return m_usernameClientMap[name];
    } else {
        throw std::range_error("Name not found: " + name);
    }

    return nullptr;
}

bool shift::FIXInitiator::isConnected()
{
    return m_connected;
}

/**
 * @brief Method to print log messages.
 *
 * @param message: a string to be print.
 */
inline void shift::FIXInitiator::debugDump(const std::string& message)
{
    if (m_verbose) {
        cout << "FIXInitiator:" << endl;
        cout << message << endl;
    }
}

/**
 * @brief Method to create internal symbols mapping
 * (e.g. AAPL.O -> AAPL)
 */
inline void shift::FIXInitiator::createSymbolMap()
{
    for (auto& originalName : m_stockList) {
        m_originalName_symbol[originalName] = originalName.substr(0, originalName.find_last_of('.'));
        m_symbol_originalName[m_originalName_symbol[originalName]] = originalName;
        // Substitute the old name with new symbol
        originalName = m_originalName_symbol[originalName];
    }
}

/**
 * @brief Method to initialize prices
 * for every symbol in the stock list
 */
inline void shift::FIXInitiator::initializePrices()
{
    for (const auto& symbol : m_stockList) {
        m_lastTrades[symbol].first = 0.0;
        m_lastTrades[symbol].second = 0;
    }

    std::lock_guard<std::mutex> opGuard(m_mtxOpenPrices);
    m_openPrices.clear();
}

/**
 * @brief Method to initialize order books
 * for every symbol in the stock list
 */
inline void shift::FIXInitiator::initializeOrderBooks()
{
    for (const auto& symbol : m_stockList) {
        if (m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_ASK]) {
            delete m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_ASK];
        }
        m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_ASK] = new OrderBookGlobalAsk(symbol);

        if (m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_BID]) {
            delete m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_BID];
        }
        m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_BID] = new OrderBookGlobalBid(symbol);

        if (m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_ASK]) {
            delete m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_ASK];
        }
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_ASK] = new OrderBookLocalAsk(symbol);

        if (m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_BID]) {
            delete m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_BID];
        }
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_BID] = new OrderBookLocalBid(symbol);
    }
}

/*
 * @brief send order book request to BC
 */
void shift::FIXInitiator::sendOrderBookRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(shift::crossguid::newGuid().str()));
    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SUBSCRIBE);
        message.setField(::FIXFIELD_MDUPDATETYPE_INCREMENTAL_REFRESH);
    } else {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_UNSUBSCRIBE);
    }
    message.setField(::FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH); // Required by FIX

    FIX50SP2::MarketDataRequest::NoMDEntryTypes entryTypeGroup1;
    FIX50SP2::MarketDataRequest::NoMDEntryTypes entryTypeGroup2;
    entryTypeGroup1.setField(::FIXFIELD_MDENTRYTYPE_BID);
    entryTypeGroup2.setField(::FIXFIELD_MDENTRYTYPE_OFFER);
    message.addGroup(entryTypeGroup1);
    message.addGroup(entryTypeGroup2);

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    relatedSymGroup.setField(FIX::Symbol(symbol));
    message.addGroup(relatedSymGroup);

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send candle data request to BC
 */
void shift::FIXInitiator::sendCandleDataRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_RFQRequest));

    message.setField(FIX::RFQReqID(shift::crossguid::newGuid().str()));

    FIX50SP2::RFQRequest::NoRelatedSym relatedSymGroup;
    relatedSymGroup.setField(FIX::Symbol(symbol));
    message.addGroup(relatedSymGroup);

    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SUBSCRIBE);
    } else {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_UNSUBSCRIBE);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method to submit Order From FIXInitiator to Brokerage Center.
 *
 * @param order as a shift::Order object contains all required information.
 * @param username as name for the user/client who is submitting the order.
 */
void shift::FIXInitiator::submitOrder(const shift::Order& order, const std::string& username)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getID()));
    message.setField(FIX::Symbol(m_symbol_originalName[order.getSymbol()]));
    message.setField(FIX::Side(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::OrdType(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(order.getPrice()));

    FIX50SP2::NewOrderSingle::NoPartyIDs partyIDGroup;
    partyIDGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    if (username != "") {
        partyIDGroup.setField(FIX::PartyID(username));
    } else {
        partyIDGroup.setField(FIX::PartyID(m_username));
    }
    message.addGroup(partyIDGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method called when a new Session was created.
 * Set Sender and Target Comp ID.
 */
void shift::FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
    debugDump("SenderID: " + s_senderID + "\nTargetID: " + s_targetID);
}

/**
 * @brief Method called when user logon to the system through
 * FIX session. Create a Logon record in the execution log.
 */
void shift::FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogon - " + sessionID.toString());

    m_logonSuccess = true;
    m_cvLogon.notify_one();

    // If there is a problem in the connection (a disconnect),
    // QuickFIX will manage the reconnection, but we need to
    // inform the BrokerageCenter "we are back in business":
    {
        // Reregister connected web client users in BrokerageCenter
        for (const auto& client : getAttachedClients()) {
            webClientSendUsername(client->getUsername());
        }

        // Resubscribe to all previously subscribed order book data
        for (const auto& symbol : getSubscribedOrderBookList()) {
            subOrderBook(symbol);
        }

        //  Resubscribe to all previously subscribed candle stick data
        for (const auto& symbol : getSubscribedCandlestickList()) {
            subCandleData(symbol);
        }
    }
}

/**
 * @brief Method called when user log out from the system.
 * Create a Logon record in the execution log.
 */
void shift::FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogout - " + sessionID.toString());
}

/**
 * @brief This Method was called when user is trying to login.
 * It sends the username and password information to Admin for verification.
 */
void shift::FIXInitiator::toAdmin(FIX::Message& message, const FIX::SessionID&) // override
{
    if (FIX::MsgType_Logon == message.getHeader().getField(FIX::FIELD::MsgType)) {
        // set username and password in logon message
        FIXT11::Logon::NoMsgTypes msgTypeGroup;
        msgTypeGroup.setField(FIX::RefMsgType(m_username));
        message.addGroup(msgTypeGroup);
        msgTypeGroup.setField(FIX::RefMsgType(m_password));
        message.addGroup(msgTypeGroup);
    }
}

/**
 * @brief Method for passing message to core client
 */
void shift::FIXInitiator::fromAdmin(const FIX::Message& message, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) // override
// void shift::FIXInitiator::fromAdmin(const FIX::Message& message)
{
    try {
        if (FIX::MsgType_Logout == message.getHeader().getField(FIX::FIELD::MsgType)) {
            FIX::Text text = message.getField(FIX::FIELD::Text);
            if (text == "Rejected Logon Attempt") {
                m_logonSuccess = false;
                m_cvLogon.notify_one();
            }
        }
    } catch (FIX::FieldNotFound&) { // Required since FIX::Text is not a mandatory field in FIX::MsgType_Logout
    }
}

/**
 * @brief Method to communicate with FIX server.
 */
void shift::FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief Receive the security list of all stock from BrokerageCenter.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID&) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    if (m_stockList.size() == 0) {
        static FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
        static FIX::Symbol symbol;

        // #pragma GCC diagnostic ignored ....

        FIX50SP2::SecurityList::NoRelatedSym* pRelatedSymGroup;
        FIX::Symbol* pSymbol;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pRelatedSymGroup = &relatedSymGroup;
            pSymbol = &symbol;
        } else { // > 1 threads; always safe way:
            pRelatedSymGroup = new decltype(relatedSymGroup);
            pSymbol = new decltype(symbol);
        }

        std::lock_guard<std::mutex> slGuard(m_mtxStockList);

        m_stockList.clear();

        for (int i = 1; i <= numOfGroups.getValue(); ++i) {
            message.getGroup(static_cast<unsigned int>(i), *pRelatedSymGroup);
            pRelatedSymGroup->get(*pSymbol);
            m_stockList.push_back(pSymbol->getValue());
        }

        createSymbolMap();
        initializePrices();
        initializeOrderBooks();

        m_cvStockList.notify_one();

        if (prevCnt) { // > 1 threads
            delete pRelatedSymGroup;
            delete pSymbol;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}

/**
 * @brief Method to receive Last Price from Brokerage Center.
 *
 * @param message as a Advertisement type object contains the last price information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::Advertisement& message, const FIX::SessionID&) // override
{
    if (m_connected) {
        static FIX::Symbol originalName;
        static FIX::Quantity size;
        static FIX::Price price;
        static FIX::TransactTime simulationTime;
        static FIX::LastMkt destination;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pOriginalName;
        FIX::Quantity* pSize;
        FIX::Price* pPrice;
        FIX::TransactTime* pSimulationTime;
        FIX::LastMkt* pDestination;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pOriginalName = &originalName;
            pSize = &size;
            pPrice = &price;
            pSimulationTime = &simulationTime;
            pDestination = &destination;
        } else { // > 1 threads; always safe way:
            pOriginalName = new decltype(originalName);
            pSize = new decltype(size);
            pPrice = new decltype(price);
            pSimulationTime = new decltype(simulationTime);
            pDestination = new decltype(destination);
        }

        message.get(*pOriginalName);
        message.get(*pSize);
        message.get(*pPrice);
        message.get(*pSimulationTime);
        message.get(*pDestination);

        std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

        m_lastTrades[symbol].first = pPrice->getValue();
        m_lastTrades[symbol].second = static_cast<int>(pSize->getValue());
        m_lastTradeTime = std::chrono::system_clock::from_time_t(pSimulationTime->getValue().getTimeT());

        try {
            getMainClient()->receiveLastPrice(symbol);
        } catch (...) {
        }

        if (prevCnt) { // > 1 threads
            delete pOriginalName;
            delete pSize;
            delete pPrice;
            delete pSimulationTime;
            delete pDestination;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}

/**
 * @brief Method to receive Global/Local orders from Brokerage Center.
 *
 * @param message as a MarketDataSnapshotFullRefresh type object contains the current accepting order information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.get(numOfEntries);
    if (numOfEntries.getValue() < 1) {
        cout << "Cannot find the Entries group in MarketDataSnapshotFullRefresh!" << endl;
        return;
    }

    static FIX::Symbol originalName;

    static FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    static FIX::MDEntryType bookType;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::MDEntryDate simulationDate;
    static FIX::MDEntryTime simulationTime;
    static FIX::MDMkt destination;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pOriginalName;

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries* pEntryGroup;
    FIX::MDEntryType* pBookType;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::MDEntryDate* pSimulationDate;
    FIX::MDEntryTime* pSimulationTime;
    FIX::MDMkt* pDestination;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOriginalName = &originalName;
        pEntryGroup = &entryGroup;
        pBookType = &bookType;
        pPrice = &price;
        pSize = &size;
        pSimulationDate = &simulationDate;
        pSimulationTime = &simulationTime;
        pDestination = &destination;
    } else { // > 1 threads; always safe way:
        pOriginalName = new decltype(originalName);
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pSimulationDate = new decltype(simulationDate);
        pSimulationTime = new decltype(simulationTime);
        pDestination = new decltype(destination);
    }

    message.getField(*pOriginalName);

    std::string symbol = m_originalName_symbol[pOriginalName->getValue()];
    std::list<shift::OrderBookEntry> orderBook;

    for (int i = 1; i <= numOfEntries.getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), *pEntryGroup);

        pEntryGroup->getField(*pBookType);
        pEntryGroup->getField(*pPrice);
        pEntryGroup->getField(*pSize);
        pEntryGroup->getField(*pSimulationDate);
        pEntryGroup->getField(*pSimulationTime);
        pEntryGroup->getField(*pDestination);

        orderBook.push_back({ static_cast<double>(pPrice->getValue()),
            static_cast<int>(pSize->getValue()),
            pDestination->getValue(),
            s_convertToTimePoint(pSimulationDate->getValue(), pSimulationTime->getValue()) });
    }

    try {
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->setOrderBook(orderBook);
    } catch (std::exception e) {
        debugDump(symbol + " doesn't work");
    }

    if (prevCnt) { // > 1 threads
        delete pOriginalName;
        delete pEntryGroup;
        delete pBookType;
        delete pPrice;
        delete pSize;
        delete pSimulationDate;
        delete pSimulationTime;
        delete pDestination;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief   Method to update already saved order book
 *          when receiving order book updates from Brokerage Center
 * @param   Message as an MarketDataIncrementalRefresh type object
 *          contains updated order book information.
 * @param   sessionID as the SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.get(numOfEntries);
    if (numOfEntries.getValue() < 1) {
        cout << "Cannot find the Entries group in MarketDataIncrementalRefresh!" << endl;
        return;
    }

    static FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    static FIX::MDEntryType bookType;
    static FIX::Symbol originalName;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::MDEntryDate simulationDate;
    static FIX::MDEntryTime simulationTime;
    static FIX::MDMkt destination;

    // #pragma GCC diagnostic ignored ....

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries* pEntryGroup;
    FIX::MDEntryType* pBookType;
    FIX::Symbol* pOriginalName;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::MDEntryDate* pSimulationDate;
    FIX::MDEntryTime* pSimulationTime;
    FIX::MDMkt* pDestination;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pEntryGroup = &entryGroup;
        pBookType = &bookType;
        pOriginalName = &originalName;
        pPrice = &price;
        pSize = &size;
        pSimulationDate = &simulationDate;
        pSimulationTime = &simulationTime;
        pDestination = &destination;
    } else { // > 1 threads; always safe way:
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pOriginalName = new decltype(originalName);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pSimulationDate = new decltype(simulationDate);
        pSimulationTime = new decltype(simulationTime);
        pDestination = new decltype(destination);
    }

    message.getGroup(1, *pEntryGroup);
    pEntryGroup->get(*pBookType);
    pEntryGroup->get(*pOriginalName);
    pEntryGroup->get(*pPrice);
    pEntryGroup->get(*pSize);
    pEntryGroup->get(*pSimulationDate);
    pEntryGroup->get(*pSimulationTime);
    pEntryGroup->get(*pDestination);

    std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

    OrderBookEntry entry{
        pPrice->getValue(),
        static_cast<int>(pSize->getValue()),
        pDestination->getValue(),
        s_convertToTimePoint(pSimulationDate->getValue(), pSimulationTime->getValue())
    };

    m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->update(std::move(entry));

    if (prevCnt) { // > 1 threads
        delete pEntryGroup;
        delete pBookType;
        delete pOriginalName;
        delete pPrice;
        delete pSize;
        delete pSimulationDate;
        delete pSimulationTime;
        delete pDestination;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to receive open/close/high/low for a specific ticker at a specific time (Data for candlestick chart).
 *
 * @param message as a SecurityStatus type object contains symbol, open, close, high, low, timestamp information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID&) // override
{
    static FIX::Symbol originalName;
    static FIX::StrikePrice openPrice;
    static FIX::HighPx highPrice;
    static FIX::LowPx lowPrice;
    static FIX::LastPx closePrice;
    static FIX::Text timestamp;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pOriginalName;
    FIX::StrikePrice* pOpenPrice;
    FIX::HighPx* pHighPrice;
    FIX::LowPx* pLowPrice;
    FIX::LastPx* pClosePrice;
    FIX::Text* pTimestamp;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOriginalName = &originalName;
        pOpenPrice = &openPrice;
        pHighPrice = &highPrice;
        pLowPrice = &lowPrice;
        pClosePrice = &closePrice;
        pTimestamp = &timestamp;
    } else { // > 1 threads; always safe way:
        pOriginalName = new decltype(originalName);
        pOpenPrice = new decltype(openPrice);
        pHighPrice = new decltype(highPrice);
        pLowPrice = new decltype(lowPrice);
        pClosePrice = new decltype(closePrice);
        pTimestamp = new decltype(timestamp);
    }

    message.get(*pOriginalName);
    message.get(*pOpenPrice);
    message.get(*pHighPrice);
    message.get(*pLowPrice);
    message.get(*pClosePrice);
    message.get(*pTimestamp);

    std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

    /**
     * @brief Logic for storing open price and check if ready. Open price stores the very first candle data open price for each ticker
     */
    if (!m_openPricesReady) {
        std::lock_guard<std::mutex> opGuard(m_mtxOpenPrices);
        if (m_openPrices.find(symbol) == m_openPrices.end()) {
            m_openPrices[symbol] = pOpenPrice->getValue();
            if (m_openPrices.size() == getStockList().size()) {
                m_openPricesReady = true;
            }
        }
    }

    try {
        getMainClient()->receiveCandlestickData(symbol, pOpenPrice->getValue(), pHighPrice->getValue(), pLowPrice->getValue(), pClosePrice->getValue(), pTimestamp->getValue());
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pOriginalName;
        delete pOpenPrice;
        delete pHighPrice;
        delete pLowPrice;
        delete pClosePrice;
        delete pTimestamp;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to receive LatestStockPrice from Brokerage Center.
 *
 * @param message as a ExecutionReport type object contains the last price information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any ClientID in ExecutionReport!" << endl;
        return;
    }

    static FIX::OrderID orderID;
    static FIX::OrdStatus status;
    static FIX::Price executedPrice;
    static FIX::CumQty executedSize;

    static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    static FIX::PartyID username;

    // #pragma GCC diagnostic ignored ....

    FIX::OrderID* pOrderID;
    FIX::OrdStatus* pStatus;
    FIX::Price* pExecutedPrice;
    FIX::CumQty* pExecutedSize;

    FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
    FIX::PartyID* pUsername;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOrderID = &orderID;
        pStatus = &status;
        pExecutedPrice = &executedPrice;
        pExecutedSize = &executedSize;
        pIDGroup = &idGroup;
        pUsername = &username;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pStatus = new decltype(status);
        pExecutedPrice = new decltype(executedPrice);
        pExecutedSize = new decltype(executedSize);
        pIDGroup = new decltype(idGroup);
        pUsername = new decltype(username);
    }

    message.get(*pOrderID);
    message.get(*pStatus);
    message.get(*pExecutedPrice);
    message.get(*pExecutedSize);

    message.getGroup(1, *pIDGroup);
    pIDGroup->get(*pUsername);

    try {
        getClient(pUsername->getValue())->storeExecution(pOrderID->getValue(), pExecutedSize->getValue(), pExecutedPrice->getValue(), static_cast<shift::Order::Status>(pStatus->getValue()));
        getClient(pUsername->getValue())->receiveExecution(pOrderID->getValue());
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pOrderID;
        delete pStatus;
        delete pExecutedPrice;
        delete pExecutedSize;
        delete pIDGroup;
        delete pUsername;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to receive portfolio item and summary from Brokerage Center.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::PositionReport& message, const FIX::SessionID&) // override
{
    while (!m_connected)
        ;

    FIX::SecurityType type;
    message.get(type);

    if (type == FIX::SecurityType_COMMON_STOCK) { // Item

        static FIX::Symbol originalName;
        static FIX::SettlPrice longPrice;
        static FIX::PriorSettlPrice shortPrice;
        static FIX::PriceDelta realizedPL;

        static FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
        static FIX::PartyID username;

        static FIX50SP2::PositionReport::NoPositions sizeGroup;
        static FIX::LongQty longSize;
        static FIX::ShortQty shortSize;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pOriginalName;
        FIX::SettlPrice* pLongPrice;
        FIX::PriorSettlPrice* pShortPrice;
        FIX::PriceDelta* pRealizedPL;

        FIX50SP2::PositionReport::NoPartyIDs* pUsernameGroup;
        FIX::PartyID* pUsername;

        FIX50SP2::PositionReport::NoPositions* pSizeGroup;
        FIX::LongQty* pLongSize;
        FIX::ShortQty* pShortSize;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pOriginalName = &originalName;
            pLongPrice = &longPrice;
            pShortPrice = &shortPrice;
            pRealizedPL = &realizedPL;
            pUsernameGroup = &usernameGroup;
            pUsername = &username;
            pSizeGroup = &sizeGroup;
            pLongSize = &longSize;
            pShortSize = &shortSize;
        } else { // > 1 threads; always safe way:
            pOriginalName = new decltype(originalName);
            pLongPrice = new decltype(longPrice);
            pShortPrice = new decltype(shortPrice);
            pRealizedPL = new decltype(realizedPL);
            pUsernameGroup = new decltype(usernameGroup);
            pUsername = new decltype(username);
            pSizeGroup = new decltype(sizeGroup);
            pLongSize = new decltype(longSize);
            pShortSize = new decltype(shortSize);
        }

        message.get(*pOriginalName);
        message.get(*pLongPrice);
        message.get(*pShortPrice);
        message.get(*pRealizedPL);

        message.getGroup(1, *pUsernameGroup);
        pUsernameGroup->get(*pUsername);

        message.getGroup(1, *pSizeGroup);
        pSizeGroup->get(*pLongSize);
        pSizeGroup->get(*pShortSize);

        // m_originalName_symbol shall be always thread-safe-readonly once after being initialized, so we shall prevent it from accidental insertion here
        if (m_originalName_symbol.find(pOriginalName->getValue()) == m_originalName_symbol.end()) {
            cout << COLOR_WARNING "FIX50SP2::PositionReport received an unknown symbol [" << pOriginalName->getValue() << "], skipped." NO_COLOR << endl;
            return;
        }

        std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

        try {
            getClient(pUsername->getValue())->storePortfolioItem(symbol, static_cast<int>(pLongSize->getValue()), static_cast<int>(pShortSize->getValue()), pLongPrice->getValue(), pShortPrice->getValue(), pRealizedPL->getValue());
            getClient(pUsername->getValue())->receivePortfolioItem(symbol);
        } catch (...) {
        }

        if (prevCnt) { // > 1 threads
            delete pOriginalName;
            delete pLongPrice;
            delete pShortPrice;
            delete pRealizedPL;
            delete pUsernameGroup;
            delete pUsername;
            delete pSizeGroup;
            delete pLongSize;
            delete pShortSize;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);

    } else { // Summary (FIX::SecurityType_CASH)

        static FIX::PriceDelta totalRealizedPL;

        static FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
        static FIX::PartyID username;

        static FIX50SP2::PositionReport::NoPositions totalSharesGroup;
        static FIX::LongQty totalShares;

        static FIX50SP2::PositionReport::NoPosAmt totalBuyingPowerGroup;
        static FIX::PosAmt totalBuyingPower;

        // #pragma GCC diagnostic ignored ....

        FIX::PriceDelta* pTotalRealizedPL;

        FIX50SP2::PositionReport::NoPartyIDs* pUsernameGroup;
        FIX::PartyID* pUsername;

        FIX50SP2::PositionReport::NoPositions* pTotalSharesGroup;
        FIX::LongQty* pTotalShares;

        FIX50SP2::PositionReport::NoPosAmt* pTotalBuyingPowerGroup;
        FIX::PosAmt* pTotalBuyingPower;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pTotalRealizedPL = &totalRealizedPL;
            pUsernameGroup = &usernameGroup;
            pUsername = &username;
            pTotalSharesGroup = &totalSharesGroup;
            pTotalShares = &totalShares;
            pTotalBuyingPowerGroup = &totalBuyingPowerGroup;
            pTotalBuyingPower = &totalBuyingPower;
        } else { // > 1 threads; always safe way:
            pTotalRealizedPL = new decltype(totalRealizedPL);
            pUsernameGroup = new decltype(usernameGroup);
            pUsername = new decltype(username);
            pTotalSharesGroup = new decltype(totalSharesGroup);
            pTotalShares = new decltype(totalShares);
            pTotalBuyingPowerGroup = new decltype(totalBuyingPowerGroup);
            pTotalBuyingPower = new decltype(totalBuyingPower);
        }

        message.get(*pTotalRealizedPL);

        message.getGroup(1, *pUsernameGroup);
        pUsernameGroup->get(*pUsername);

        message.getGroup(1, *pTotalSharesGroup);
        pTotalSharesGroup->get(*pTotalShares);

        message.getGroup(1, *pTotalBuyingPowerGroup);
        pTotalBuyingPowerGroup->get(*pTotalBuyingPower);

        try {
            getClient(pUsername->getValue())->storePortfolioSummary(pTotalBuyingPower->getValue(), static_cast<int>(pTotalShares->getValue()), pTotalRealizedPL->getValue());
            getClient(pUsername->getValue())->receivePortfolioSummary();
        } catch (...) {
        }

        if (prevCnt) { // > 1 threads
            delete pTotalRealizedPL;
            delete pUsernameGroup;
            delete pUsername;
            delete pTotalSharesGroup;
            delete pTotalShares;
            delete pTotalBuyingPowerGroup;
            delete pTotalBuyingPower;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}

/**
 * @brief Method to receive waiting list from Brokerage Center.
 *
 * @param message as a QuoteAcknowledgement type object contains the current waiting list information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID&) // override
{
    while (!m_connected)
        ;

    static FIX::Account username;
    static FIX::NoQuoteSets n;

    static FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;
    static FIX::QuoteSetID orderID;
    static FIX::UnderlyingSymbol originalName;
    static FIX::UnderlyingOptAttribute orderType;
    static FIX::UnderlyingQty size;
    static FIX::UnderlyingPx price;
    static FIX::UnderlyingAdjustedQuantity executedSize;
    static FIX::UnderlyingFXRateCalc status;

    // #pragma GCC diagnostic ignored ....

    FIX::Account* pUsername;
    FIX::NoQuoteSets* pN;

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets* pQuoteSetGroup;
    FIX::QuoteSetID* pOrderID;
    FIX::UnderlyingSymbol* pOriginalName;
    FIX::UnderlyingOptAttribute* pOrderType;
    FIX::UnderlyingQty* pSize;
    FIX::UnderlyingPx* pPrice;
    FIX::UnderlyingAdjustedQuantity* pExecutedSize;
    FIX::UnderlyingFXRateCalc* pStatus;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pUsername = &username;
        pN = &n;
        pQuoteSetGroup = &quoteSetGroup;
        pOrderID = &orderID;
        pOriginalName = &originalName;
        pOrderType = &orderType;
        pSize = &size;
        pPrice = &price;
        pExecutedSize = &executedSize;
        pStatus = &status;
    } else { // > 1 threads; always safe way:
        pUsername = new decltype(username);
        pN = new decltype(n);
        pQuoteSetGroup = new decltype(quoteSetGroup);
        pOrderID = new decltype(orderID);
        pOriginalName = new decltype(originalName);
        pOrderType = new decltype(orderType);
        pSize = new decltype(size);
        pPrice = new decltype(price);
        pExecutedSize = new decltype(executedSize);
        pStatus = new decltype(status);
    }

    message.get(*pUsername);
    message.get(*pN);

    std::vector<shift::Order> waitingList;

    for (int i = 1; i <= pN->getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), *pQuoteSetGroup);

        pQuoteSetGroup->get(*pOrderID);
        pQuoteSetGroup->get(*pOriginalName);
        pQuoteSetGroup->get(*pOrderType);
        pQuoteSetGroup->get(*pSize);
        pQuoteSetGroup->get(*pPrice);
        pQuoteSetGroup->get(*pExecutedSize);
        pQuoteSetGroup->get(*pStatus);

        int sizeInt = static_cast<int>(pSize->getValue());

        if (sizeInt > 0) {

            // m_originalName_symbol shall be always thread-safe-readonly once after being initialized, so we shall prevent it from accidental insertion here
            if (m_originalName_symbol.find(pOriginalName->getValue()) == m_originalName_symbol.end()) {
                cout << COLOR_WARNING "FIX50SP2::PositionReport received an unknown symbol [" << pOriginalName->getValue() << "], skipped." NO_COLOR << endl;
                continue;
            }

            std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

            shift::Order order{
                static_cast<shift::Order::Type>(pOrderType->getValue()),
                symbol,
                sizeInt,
                pPrice->getValue(),
                pOrderID->getValue()
            };
            order.setExecutedSize(static_cast<int>(pExecutedSize->getValue()));
            order.setStatus(static_cast<shift::Order::Status>(pStatus->getValue()));

            waitingList.push_back(std::move(order));
        }
    }

    try {
        getClient(username.getValue())->storeWaitingList(std::move(waitingList));
        getClient(username.getValue())->receiveWaitingList();
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pUsername;
        delete pN;
        delete pQuoteSetGroup;
        delete pOrderID;
        delete pOriginalName;
        delete pOrderType;
        delete pSize;
        delete pPrice;
        delete pExecutedSize;
        delete pStatus;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to get the open price of a certain symbol. Return the value from the open price book map.
 *
 * @param symbol The name of the symbol to be searched as a string.
 * @return The result open price as a double.
 */
double shift::FIXInitiator::getOpenPrice(const std::string& symbol)
{
    std::lock_guard<std::mutex> opGuard(m_mtxOpenPrices);
    if (m_openPrices.find(symbol) != m_openPrices.end()) {
        return m_openPrices[symbol];
    } else {
        return 0.0;
    }
}

/**
 * @brief Method to get the last traded price of a certain symbol. Return the value from the last trades map.
 *
 * @param symbol The symbol to be searched as a string.
 * @return The result last price as a double.
 */
double shift::FIXInitiator::getLastPrice(const std::string& symbol)
{
    return m_lastTrades[symbol].first;
}

/**
 * @brief Method to get the last traded size of a certain symbol. Return the value from the last trades map.
 *
 * @param symbol The symbol to be searched as a string.
 * @return The result last size as an int.
 */
int shift::FIXInitiator::getLastSize(const std::string& symbol)
{
    return m_lastTrades[symbol].second;
}

/**
 * @brief Method to get the time of the last trade.
 *
 * @return The result time of the last trade.
 */
std::chrono::system_clock::time_point shift::FIXInitiator::getLastTradeTime()
{
    return m_lastTradeTime;
}

/**
 * @brief Method to get the BestPrice object for the target symbol.
 *
 * @param symbol The name of the symbol to be searched.
 *
 * @return shift::BestPrice for the target symbol.
 */
shift::BestPrice shift::FIXInitiator::getBestPrice(const std::string& symbol)
{
    shift::BestPrice bp = shift::BestPrice(m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_BID]->getBestPrice(),
        m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_BID]->getBestSize(),
        m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_ASK]->getBestPrice(),
        m_orderBooks[symbol][shift::OrderBook::Type::GLOBAL_ASK]->getBestSize(),
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_BID]->getBestPrice(),
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_BID]->getBestSize(),
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_ASK]->getBestPrice(),
        m_orderBooks[symbol][shift::OrderBook::Type::LOCAL_ASK]->getBestSize());

    return bp;
}

/**
 * @brief Method to get the corresponding order book by symbol name and entry type.
 *
 * @param symbol The target symbol to find from the order book map.
 *
 * @param type The target entry type (Global bid "B"/ask "A", local bid "b"/ask "a")
 */
std::vector<shift::OrderBookEntry> shift::FIXInitiator::getOrderBook(const std::string& symbol, OrderBook::Type type, int maxLevel)
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Order Book for symbol " + symbol;
    }

    if (m_orderBooks[symbol].find(type) == m_orderBooks[symbol].end()) {
        throw "Order Book type is invalid";
    }

    return m_orderBooks[symbol][type]->getOrderBook(maxLevel);
}

/**
 * @brief Method to get the corresponding order book with destination by symbol name and entry type.
 *
 * @param symbol The target symbol to find from the order book map.
 *
 * @param type The target entry type (Global bid "B"/ask "A", local bid "b"/ask "a")
 */
std::vector<shift::OrderBookEntry> shift::FIXInitiator::getOrderBookWithDestination(const std::string& symbol, OrderBook::Type type)
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Order Book for symbol " + symbol;
    }
    if (m_orderBooks[symbol].find(type) == m_orderBooks[symbol].end()) {
        throw "Order Book type is invalid";
    }

    return m_orderBooks[symbol][type]->getOrderBookWithDestination();
}

/**
 * @brief Method to get the current stock list.
 *
 * @return A vector contains all symbols for this session.
 */
std::vector<std::string> shift::FIXInitiator::getStockList()
{
    std::lock_guard<std::mutex> slGuard(m_mtxStockList);
    return m_stockList;
}

void shift::FIXInitiator::fetchCompanyName(std::string tickerName)
{
    // Find the target url
    std::string modifiedName;
    std::regex nameTrimmer("\\.[^.]+$");

    if (tickerName.find(".")) {
        modifiedName = std::regex_replace(tickerName, nameTrimmer, "");
    } else {
        modifiedName = tickerName;
    }

    std::string url = "https://finance.yahoo.com/quote/" + modifiedName + "?p=" + modifiedName;

    // Start using curl to crawl names down from yahoo
    CURL* curl;
    std::string html;
    std::string companyName;

    curl = curl_easy_init(); // Initilise web query

    if (curl) {
        typedef size_t (*CURL_WRITEFUNCTION_PTR)(char*, size_t, size_t, std::string*);
        auto sWriter = [](char* data, size_t size, size_t nmemb, std::string* buffer) {
            size_t result = 0;

            if (buffer != nullptr) {
                buffer->append(data, size * nmemb);
                result = size * nmemb;
            }

            return result;
        };
        // Since curl is a C library, need to cast url into c string to process, same for HTML
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITEFUNCTION_PTR>(sWriter)); // manages the required buffer size
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html); // Data Pointer HTML stores downloaded web content
    } else {
        debugDump("Error creating curl object!");
    }

    if (curl_easy_perform(curl)) {
        debugDump("Error reading webpage!");
        curl_easy_cleanup(curl);
    } else {
        curl_easy_cleanup(curl); // Close connection

        std::regex findingRegex("(<h1.*?>)(.*?) (\\()");
        std::smatch matchs;

        if (std::regex_search(html, matchs, findingRegex)) {
            companyName = matchs[2].str();
        }
    }

    companyName = std::regex_replace(companyName, std::regex("&amp;"), "&");
    companyName = std::regex_replace(companyName, std::regex("&#x27;"), "'");

    std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
    m_companyNames[tickerName] = companyName;
}

void shift::FIXInitiator::requestCompanyNames()
{
    for (const auto& symbol : getStockList()) {
        std::thread(&shift::FIXInitiator::fetchCompanyName, this, symbol).detach();
    }
}

std::map<std::string, std::string> shift::FIXInitiator::getCompanyNames()
{
    std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
    return m_companyNames;
}

std::string shift::FIXInitiator::getCompanyName(const std::string& symbol)
{
    std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
    return m_companyNames[symbol];
}

void shift::FIXInitiator::subOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    // It's ok to send repeated subscription requests.
    // A test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail.
    m_subscribedOrderBookSet.insert(symbol);
    sendOrderBookRequest(m_symbol_originalName[symbol], true);
}

/**
 * @brief unsubscriber the orderbook
 */
void shift::FIXInitiator::unsubOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    if (m_subscribedOrderBookSet.find(symbol) != m_subscribedOrderBookSet.end()) {
        m_subscribedOrderBookSet.erase(symbol);
        sendOrderBookRequest(m_symbol_originalName[symbol], false);
    }
}

void shift::FIXInitiator::subAllOrderBook()
{
    for (const auto& symbol : getStockList()) {
        subOrderBook(symbol);
    }
}

void shift::FIXInitiator::unsubAllOrderBook()
{
    for (const auto& symbol : getStockList()) {
        unsubOrderBook(symbol);
    }
}

/**
 * @brief Method to get the list of symbols who currently
 * subscribed their order book.
 *
 * @return  A vector of symbols who subscribed their order book.
 */
std::vector<std::string> shift::FIXInitiator::getSubscribedOrderBookList()
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedOrderBookSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}

void shift::FIXInitiator::subCandleData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandleStickSet);

    // It's ok to send repeated subscription requests.
    // A test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail.
    m_subscribedCandleStickSet.insert(symbol);
    sendCandleDataRequest(m_symbol_originalName[symbol], true);
}

void shift::FIXInitiator::unsubCandleData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandleStickSet);

    if (m_subscribedCandleStickSet.find(symbol) != m_subscribedCandleStickSet.end()) {
        m_subscribedCandleStickSet.erase(symbol);
        sendCandleDataRequest(m_symbol_originalName[symbol], false);
    }
}

void shift::FIXInitiator::subAllCandleData()
{
    for (const auto& symbol : getStockList()) {
        subCandleData(symbol);
    }
}

void shift::FIXInitiator::unsubAllCandleData()
{
    for (const auto& symbol : getStockList()) {
        unsubCandleData(symbol);
    }
}

/**
 * @brief Method to get the list of symbols who currently
 * subscribed their candle data.
 *
 * @return A vector of symbols who subscribed their candle data.
 */
std::vector<std::string> shift::FIXInitiator::getSubscribedCandlestickList()
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandleStickSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedCandleStickSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}
