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
#include <Common.h>
#include <crypto/Encryptor.h>
#include <fix/HelperFunctions.h>
#include <terminal/Common.h>
#else
#include <shift/miscutils/Common.h>
#include <shift/miscutils/crypto/Encryptor.h>
#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>
#endif

#define CSTR_BAD_USERID "BAD_USERID"

using namespace std::chrono_literals;

/* static */ std::string shift::FIXInitiator::s_senderID;
/* static */ std::string shift::FIXInitiator::s_targetID;

// predefined constant FIX message fields (to avoid recalculations):
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
    : m_connected(false)
    , m_verbose(false)
    , m_logonSuccess(false)
    , m_openPricesReady(false)
    , m_lastTradeTime(std::chrono::system_clock::time_point())
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
        std::lock_guard<std::mutex> guard(m_mtxClientByUserID);
        m_clientByUserID.clear();
    }

    disconnectBrokerageCenter();
}

/**
 * @brief Get singleton instance of FIXInitiator.
 */
shift::FIXInitiator& shift::FIXInitiator::getInstance()
{
    static FIXInitiator fixInitiator;
    return fixInitiator;
}

void shift::FIXInitiator::connectBrokerageCenter(const std::string& cfgFile, CoreClient* client, const std::string& password, bool verbose, int timeout)
{
    disconnectBrokerageCenter();

    m_superUsername = client->getUsername();

    std::istringstream iss{ password };
    shift::crypto::Encryptor enc;
    iss >> enc >> m_superUserPsw;

    m_verbose = verbose;

    std::string senderCompID = shift::toUpper(m_superUsername);

    FIX::SessionSettings settings(cfgFile);
    const FIX::Dictionary& commonDict = settings.get();
    FIX::SessionID sessionID(commonDict.getString("BeginString"),
        senderCompID,
        commonDict.getString("TargetCompID"));

    FIX::Dictionary dict;
    settings.set(sessionID, std::move(dict));

    if (commonDict.has("FileLogPath")) {
        m_pLogFactory.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath") + "/" + m_superUsername));
    } else {
        m_pLogFactory.reset(new FIX::ScreenLogFactory(false, false, m_verbose));
    }

    if (commonDict.has("FileStorePath")) {
        m_pMessageStoreFactory.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath") + "/" + m_superUsername));
    } else {
        m_pMessageStoreFactory.reset(new FIX::NullStoreFactory());
    }

    m_pSocketInitiator.reset(new FIX::SocketInitiator(shift::FIXInitiator::getInstance(), *m_pMessageStoreFactory, settings, *m_pLogFactory));
    m_pSocketInitiator->start();

    bool wasNotified = false;
    {
        std::unique_lock<std::mutex> lsUniqueLock(m_mtxLogon);
        auto t1 = std::chrono::steady_clock::now();
        // gets notify from fromAdmin() or onLogon(), or timeout if backend cannot resolve(e.g. no such user in DB)
        m_cvLogon.wait_for(lsUniqueLock, timeout * 1ms);
        auto t2 = std::chrono::steady_clock::now();

        wasNotified = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() < timeout;
    }

    if (m_logonSuccess) {
        std::unique_lock<std::mutex> slUniqueLock(m_mtxStockList);
        // gets notify when security list is ready
        m_cvStockList.wait_for(slUniqueLock, timeout * 1ms);

        if (!attachClient(client)) {
            m_connected = false;
            return;
        }

        m_superUserID = client->getUserID();
        m_connected = m_logonSuccess.load();
    } else {
        disconnectBrokerageCenter();

        if (wasNotified) {
            throw shift::IncorrectPasswordError();
        } else {
            throw shift::ConnectionTimeoutError();
        }
    }
}

void shift::FIXInitiator::disconnectBrokerageCenter()
{
    // stop QuickFIX
    if (m_pSocketInitiator) {
        m_pSocketInitiator->stop();
        m_pSocketInitiator = nullptr;
    }
    m_pMessageStoreFactory = nullptr;
    m_pLogFactory = nullptr;

    // house cleaning:

    // cancel all pending client requests
    try {
        getSuperUser()->cancelAllSamplePricesRequests();
    } catch (...) { // it is OK if there is no main client yet
    } // (we force a disconnect before every connection - even the first one)

    for (auto& client : getAttachedClients()) {
        client->cancelAllSamplePricesRequests();
    }

    // reset state variables:
    m_connected = false;
    m_verbose = false;
    m_logonSuccess = false;
    m_openPricesReady = false;
    m_lastTradeTime = std::chrono::system_clock::time_point();

    // stock list is only updated when first connecting,
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

    // subscriptions will reset automatically when disconnecting,
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
 * Don't call this function directly.
 */
bool shift::FIXInitiator::attachClient(shift::CoreClient* client, const std::string& password)
{
    // TODO: add auth in BC; use password to auth

    if (!client || !registerUserInBCWaitResponse(client)) {
        cout << COLOR_ERROR << "Attach to FIXInitiator failed for client [" << client->getUsername() << "]." NO_COLOR << endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> guard(m_mtxClientByUserID);
        m_clientByUserID[client->getUserID()] = client;
    }

    return client->attachInitiator(*this);
}

bool shift::FIXInitiator::registerUserInBCWaitResponse(shift::CoreClient* client)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserRequest));

    message.setField(FIX::UserRequestID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_USERREQUESTTYPE_LOG_ON_USER); // Required by FIX
    message.setField(FIX::Username(client->getUsername()));

    FIX::Session::sendToTarget(message);

    std::unique_lock<std::mutex> lock(m_mtxUserIDByUsername);
    m_cvUserIDByUsername.wait(lock, [this, client] { return m_userIDByUsername[client->getUsername()].length(); }); // wait for userID update event

    const auto& userID = m_userIDByUsername[client->getUsername()];
    if (userID == CSTR_BAD_USERID) {
        cout << COLOR_ERROR "ERROR: Register user [" << client->getUsername() << "] in BC cannot resolve User ID. Registration is rejected." NO_COLOR << endl;
        return false;
    }

    client->setUserID(userID);
    return true;
}

std::vector<shift::CoreClient*> shift::FIXInitiator::getAttachedClients()
{
    std::vector<shift::CoreClient*> clientsVector;

    std::lock_guard<std::mutex> guard(m_mtxClientByUserID);

    for (const auto& kv : m_clientByUserID) {
        if (kv.first != m_superUserID) { // skip MainClient/super user
            clientsVector.push_back(kv.second);
        }
    }

    return clientsVector;
}

shift::CoreClient* shift::FIXInitiator::getClient(const std::string& username)
{
    std::string userID;
    {
        std::lock_guard<std::mutex> guard(m_mtxUserIDByUsername);
        userID = m_userIDByUsername[username];
    }

    return getClientByUserID(userID);
}

shift::CoreClient* shift::FIXInitiator::getClientByUserID(const std::string& userID)
{
    std::lock_guard<std::mutex> guard(m_mtxClientByUserID);

    if (m_clientByUserID.find(userID) != m_clientByUserID.end()) {
        return m_clientByUserID[userID];
    } else {
        throw std::range_error("User not found: " + userID);
    }

    return nullptr;
}

shift::CoreClient* shift::FIXInitiator::getSuperUser()
{
    return getClientByUserID(m_superUserID);
}

bool shift::FIXInitiator::isConnected()
{
    return m_connected;
}

/**
 * @brief Method to print log messages.
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
 * @brief Method to create internal symbols mapping (e.g. AAPL.O -> AAPL).
 */
inline void shift::FIXInitiator::createSymbolMap()
{
    for (auto& originalName : m_stockList) {
        m_originalName_symbol[originalName] = originalName.substr(0, originalName.find_last_of('.'));
        m_symbol_originalName[m_originalName_symbol[originalName]] = originalName;
        // substitute the old name with new symbol
        originalName = m_originalName_symbol[originalName];
    }
}

/**
 * @brief Method to initialize prices for every symbol in the stock list.
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
 * @brief Method to initialize order books for every symbol in the stock list.
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
 * @brief send order book request to BC.
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
    message.setField(::FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH); // required by FIX

    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message,
        ::FIXFIELD_MDENTRYTYPE_BID);
    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message,
        ::FIXFIELD_MDENTRYTYPE_OFFER);
    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoRelatedSym>(message,
        FIX::Symbol(symbol));

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send candle data request to BC.
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

    shift::fix::addFIXGroup<FIX50SP2::RFQRequest::NoRelatedSym>(message,
        FIX::Symbol(symbol));

    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SUBSCRIBE);
    } else {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_UNSUBSCRIBE);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method to submit Order From FIXInitiator to Brokerage Center.
 * @param order as a shift::Order object contains all required information.
 * @param userID as identifier for the user/client who is submitting the order.
 */
void shift::FIXInitiator::submitOrder(const shift::Order& order, const std::string& userID /*= ""*/)
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

    shift::fix::addFIXGroup<FIX50SP2::NewOrderSingle::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        userID != "" ? FIX::PartyID(userID) : FIX::PartyID(m_superUserID));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method called when a new Session was created. Set Sender and Target Comp ID.
 */
void shift::FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
    debugDump("SenderID: " + s_senderID + "\nTargetID: " + s_targetID);
}

/**
 * @brief Method called when user logon to the system through FIX session. Create a Logon record in the execution log.
 */
void shift::FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogon - " + sessionID.toString());

    m_logonSuccess = true;
    m_cvLogon.notify_one();

    // if there is a problem in the connection (a disconnect),
    // QuickFIX will manage the reconnection, but we need to
    // inform the BrokerageCenter "we are back in business":
    {
        // reregister super user in BrokerageCenter
        if(!registerUserInBCWaitResponse(getSuperUser()))
            std::terminate(); // precondition broken: super user shall not fail when reregistering

        // reregister connected web client users in BrokerageCenter
        for (const auto& client : getAttachedClients()) {
            if (!registerUserInBCWaitResponse(client))
                std::terminate(); // precondition broken: attached clients (by attachClient()) shall not fail when reregistering
        }

        // resubscribe to all previously subscribed order book data
        for (const auto& symbol : getSubscribedOrderBookList()) {
            subOrderBook(symbol);
        }

        // resubscribe to all previously subscribed candle stick data
        for (const auto& symbol : getSubscribedCandlestickList()) {
            subCandleData(symbol);
        }
    }
}

/**
 * @brief Method called when user log out from the system. Create a Logon record in the execution log.
 */
void shift::FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogout - " + sessionID.toString());
}

/**
 * @brief This Method was called when user is trying to login. It sends the username and password information to Admin for verification.
 */
void shift::FIXInitiator::toAdmin(FIX::Message& message, const FIX::SessionID&) // override
{
    if (FIX::MsgType_Logon == message.getHeader().getField(FIX::FIELD::MsgType)) {
        shift::fix::addFIXGroup<FIXT11::Logon::NoMsgTypes>(message,
            FIX::RefMsgType(m_superUsername));
        shift::fix::addFIXGroup<FIXT11::Logon::NoMsgTypes>(message,
            FIX::RefMsgType(m_superUserPsw));
    }
}

/**
 * @brief Method for passing message to Core Client.
 */
void shift::FIXInitiator::fromAdmin(const FIX::Message& message, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) // override
{
    try {
        if (FIX::MsgType_Logout == message.getHeader().getField(FIX::FIELD::MsgType)) {
            FIX::Text text = message.getField(FIX::FIELD::Text);
            if (text == "Rejected Logon Attempt") {
                m_logonSuccess = false;
                m_cvLogon.notify_one();
            }
        }
    } catch (const FIX::FieldNotFound&) { // required since FIX::Text is not a mandatory field in FIX::MsgType_Logout
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
 * @brief Receive the security list from BrokerageCenter.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID&) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    if (m_stockList.size() == 0) {
        FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
        FIX::Symbol symbol;

        std::lock_guard<std::mutex> slGuard(m_mtxStockList);

        m_stockList.clear();
        for (int i = 1; i <= numOfGroups.getValue(); ++i) {
            message.getGroup(static_cast<unsigned int>(i), relatedSymGroup);
            relatedSymGroup.getField(symbol);
            m_stockList.push_back(symbol.getValue());
        }

        createSymbolMap();
        initializePrices();
        initializeOrderBooks();

        m_cvStockList.notify_one();
    }
}

/**
 * @brief Receive user ID from BrokerageCenter.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::UserResponse& message, const FIX::SessionID&) // override
{
    FIX::Username username;
    FIX::UserStatus userStatus;
    FIX::UserStatusText userID;

    message.getField(username);
    message.getField(userStatus);
    message.getField(userID);

    {
        std::lock_guard<std::mutex> guard(m_mtxUserIDByUsername);
        m_userIDByUsername[username.getValue()] = (1 == userStatus) ? userID.getValue() : std::string(CSTR_BAD_USERID);
    }

    m_cvUserIDByUsername.notify_all();
}

/**
 * @brief Method to receive Last Price from Brokerage Center.
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
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

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

        message.getField(*pOriginalName);
        message.getField(*pSize);
        message.getField(*pPrice);
        message.getField(*pSimulationTime);
        message.getField(*pDestination);

        std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

        m_lastTrades[symbol].first = pPrice->getValue();
        m_lastTrades[symbol].second = static_cast<int>(pSize->getValue());
        m_lastTradeTime = std::chrono::system_clock::from_time_t(pSimulationTime->getValue().getTimeT());

        try {
            getSuperUser()->receiveLastPrice(symbol);
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
 * @param message as a MarketDataSnapshotFullRefresh type object contains the current accepting order information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.getField(numOfEntries);
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
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

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
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->setOrderBook(std::move(orderBook));
    } catch (const std::exception&) {
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
 * @brief   Method to update already saved order book when receiving order book updates from Brokerage Center
 * @param   Message as an MarketDataIncrementalRefresh type object contains updated order book information.
 * @param   sessionID as the SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.getField(numOfEntries);
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
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

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
    pEntryGroup->getField(*pBookType);
    pEntryGroup->getField(*pOriginalName);
    pEntryGroup->getField(*pPrice);
    pEntryGroup->getField(*pSize);
    pEntryGroup->getField(*pSimulationDate);
    pEntryGroup->getField(*pSimulationTime);
    pEntryGroup->getField(*pDestination);

    std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

    if (pPrice->getValue() > 0.0) {
        OrderBookEntry entry{
            pPrice->getValue(),
            static_cast<int>(pSize->getValue()),
            pDestination->getValue(),
            s_convertToTimePoint(pSimulationDate->getValue(), pSimulationTime->getValue())
        };
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->update(std::move(entry));
    } else {
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->resetOrderBook();
    }

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
 * @param message as a SecurityStatus type object contains symbol, open, close, high, low, timestamp information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID&) // override
{
    static FIX::Symbol originalName;
    static FIX::HighPx highPrice;
    static FIX::LowPx lowPrice;
    static FIX::LastPx closePrice;
    static FIX::FirstPx openPrice;
    static FIX::TransactTime timestamp;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pOriginalName;
    FIX::HighPx* pHighPrice;
    FIX::LowPx* pLowPrice;
    FIX::LastPx* pClosePrice;
    FIX::FirstPx* pOpenPrice;
    FIX::TransactTime* pTimestamp;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOriginalName = &originalName;
        pHighPrice = &highPrice;
        pLowPrice = &lowPrice;
        pClosePrice = &closePrice;
        pOpenPrice = &openPrice;
        pTimestamp = &timestamp;
    } else { // > 1 threads; always safe way:
        pOriginalName = new decltype(originalName);
        pHighPrice = new decltype(highPrice);
        pLowPrice = new decltype(lowPrice);
        pClosePrice = new decltype(closePrice);
        pOpenPrice = new decltype(openPrice);
        pTimestamp = new decltype(timestamp);
    }

    message.getField(*pOriginalName);
    message.getField(*pHighPrice);
    message.getField(*pLowPrice);
    message.getField(*pClosePrice);
    message.getField(*pOpenPrice);
    message.getField(*pTimestamp);

    std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

    // logic for storing open price and check if ready:
    // open price stores the very first candle data open price for each ticker
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
        getSuperUser()->receiveCandlestickData(symbol, pOpenPrice->getValue(), pHighPrice->getValue(), pLowPrice->getValue(), pClosePrice->getValue(), std::to_string(pTimestamp->getValue().getTimeT()));
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pOriginalName;
        delete pHighPrice;
        delete pLowPrice;
        delete pClosePrice;
        delete pOpenPrice;
        delete pTimestamp;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to receive Execution Reports from Brokerage Center.
 * @param message as a ExecutionReport type object contains the last execution report.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any ClientID in ExecutionReport!" << endl;
        return;
    }

    static FIX::OrderID orderID;
    static FIX::OrdStatus status;
    static FIX::OrdType orderType;
    static FIX::Price executedPrice;
    static FIX::CumQty executedSize;

    static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    static FIX::PartyID userID;

    // #pragma GCC diagnostic ignored ....

    FIX::OrderID* pOrderID;
    FIX::OrdStatus* pStatus;
    FIX::OrdType* pOrderType;
    FIX::Price* pExecutedPrice;
    FIX::CumQty* pExecutedSize;

    FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
    FIX::PartyID* pUserID;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOrderID = &orderID;
        pStatus = &status;
        pOrderType = &orderType;
        pExecutedPrice = &executedPrice;
        pExecutedSize = &executedSize;
        pIDGroup = &idGroup;
        pUserID = &userID;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pStatus = new decltype(status);
        pOrderType = new decltype(orderType);
        pExecutedPrice = new decltype(executedPrice);
        pExecutedSize = new decltype(executedSize);
        pIDGroup = new decltype(idGroup);
        pUserID = new decltype(userID);
    }

    message.getField(*pOrderID);
    message.getField(*pStatus);
    message.getField(*pOrderType);
    message.getField(*pExecutedPrice);
    message.getField(*pExecutedSize);

    message.getGroup(1, *pIDGroup);
    pIDGroup->getField(*pUserID);

    try {
        getClientByUserID(pUserID->getValue())->storeExecution(pOrderID->getValue(), static_cast<shift::Order::Type>(pOrderType->getValue()), pExecutedSize->getValue(), pExecutedPrice->getValue(), static_cast<shift::Order::Status>(pStatus->getValue()));
        getClientByUserID(pUserID->getValue())->receiveExecution(pOrderID->getValue());
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pOrderID;
        delete pStatus;
        delete pExecutedPrice;
        delete pExecutedSize;
        delete pIDGroup;
        delete pUserID;
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
        std::this_thread::sleep_for(10ms);

    FIX::SecurityType type;
    message.getField(type);

    if (type == FIX::SecurityType_COMMON_STOCK) { // item

        static FIX::Symbol originalName;
        static FIX::SettlPrice longPrice;
        static FIX::PriorSettlPrice shortPrice;
        static FIX::PriceDelta realizedPL;

        static FIX50SP2::PositionReport::NoPartyIDs userIDGroup;
        static FIX::PartyID userID;

        static FIX50SP2::PositionReport::NoPositions sizeGroup;
        static FIX::LongQty longSize;
        static FIX::ShortQty shortSize;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pOriginalName;
        FIX::SettlPrice* pLongPrice;
        FIX::PriorSettlPrice* pShortPrice;
        FIX::PriceDelta* pRealizedPL;

        FIX50SP2::PositionReport::NoPartyIDs* pUserIDGroup;
        FIX::PartyID* pUserID;

        FIX50SP2::PositionReport::NoPositions* pSizeGroup;
        FIX::LongQty* pLongSize;
        FIX::ShortQty* pShortSize;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pOriginalName = &originalName;
            pLongPrice = &longPrice;
            pShortPrice = &shortPrice;
            pRealizedPL = &realizedPL;
            pUserIDGroup = &userIDGroup;
            pUserID = &userID;
            pSizeGroup = &sizeGroup;
            pLongSize = &longSize;
            pShortSize = &shortSize;
        } else { // > 1 threads; always safe way:
            pOriginalName = new decltype(originalName);
            pLongPrice = new decltype(longPrice);
            pShortPrice = new decltype(shortPrice);
            pRealizedPL = new decltype(realizedPL);
            pUserIDGroup = new decltype(userIDGroup);
            pUserID = new decltype(userID);
            pSizeGroup = new decltype(sizeGroup);
            pLongSize = new decltype(longSize);
            pShortSize = new decltype(shortSize);
        }

        message.getField(*pOriginalName);
        message.getField(*pLongPrice);
        message.getField(*pShortPrice);
        message.getField(*pRealizedPL);

        message.getGroup(1, *pUserIDGroup);
        pUserIDGroup->getField(*pUserID);

        message.getGroup(1, *pSizeGroup);
        pSizeGroup->getField(*pLongSize);
        pSizeGroup->getField(*pShortSize);

        // m_originalName_symbol shall be always thread-safe-readonly once after being initialized, so we shall prevent it from accidental insertion here
        if (m_originalName_symbol.find(pOriginalName->getValue()) == m_originalName_symbol.end()) {
            cout << COLOR_WARNING "FIX50SP2::PositionReport received an unknown symbol [" << pOriginalName->getValue() << "], skipped." NO_COLOR << endl;
            return;
        }

        std::string symbol = m_originalName_symbol[pOriginalName->getValue()];

        try {
            getClientByUserID(pUserID->getValue())->storePortfolioItem(symbol, static_cast<int>(pLongSize->getValue()), static_cast<int>(pShortSize->getValue()), pLongPrice->getValue(), pShortPrice->getValue(), pRealizedPL->getValue());
            getClientByUserID(pUserID->getValue())->receivePortfolioItem(symbol);
        } catch (...) {
        }

        if (prevCnt) { // > 1 threads
            delete pOriginalName;
            delete pLongPrice;
            delete pShortPrice;
            delete pRealizedPL;
            delete pUserIDGroup;
            delete pUserID;
            delete pSizeGroup;
            delete pLongSize;
            delete pShortSize;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);

    } else { // summary (FIX::SecurityType_CASH)

        static FIX::PriceDelta totalRealizedPL;

        static FIX50SP2::PositionReport::NoPartyIDs userIDGroup;
        static FIX::PartyID userID;

        static FIX50SP2::PositionReport::NoPositions totalSharesGroup;
        static FIX::LongQty totalShares;

        static FIX50SP2::PositionReport::NoPosAmt totalBuyingPowerGroup;
        static FIX::PosAmt totalBuyingPower;

        // #pragma GCC diagnostic ignored ....

        FIX::PriceDelta* pTotalRealizedPL;

        FIX50SP2::PositionReport::NoPartyIDs* pUserIDGroup;
        FIX::PartyID* pUserID;

        FIX50SP2::PositionReport::NoPositions* pTotalSharesGroup;
        FIX::LongQty* pTotalShares;

        FIX50SP2::PositionReport::NoPosAmt* pTotalBuyingPowerGroup;
        FIX::PosAmt* pTotalBuyingPower;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pTotalRealizedPL = &totalRealizedPL;
            pUserIDGroup = &userIDGroup;
            pUserID = &userID;
            pTotalSharesGroup = &totalSharesGroup;
            pTotalShares = &totalShares;
            pTotalBuyingPowerGroup = &totalBuyingPowerGroup;
            pTotalBuyingPower = &totalBuyingPower;
        } else { // > 1 threads; always safe way:
            pTotalRealizedPL = new decltype(totalRealizedPL);
            pUserIDGroup = new decltype(userIDGroup);
            pUserID = new decltype(userID);
            pTotalSharesGroup = new decltype(totalSharesGroup);
            pTotalShares = new decltype(totalShares);
            pTotalBuyingPowerGroup = new decltype(totalBuyingPowerGroup);
            pTotalBuyingPower = new decltype(totalBuyingPower);
        }

        message.getField(*pTotalRealizedPL);

        message.getGroup(1, *pUserIDGroup);
        pUserIDGroup->getField(*pUserID);

        message.getGroup(1, *pTotalSharesGroup);
        pTotalSharesGroup->getField(*pTotalShares);

        message.getGroup(1, *pTotalBuyingPowerGroup);
        pTotalBuyingPowerGroup->getField(*pTotalBuyingPower);

        try {
            getClientByUserID(pUserID->getValue())->storePortfolioSummary(pTotalBuyingPower->getValue(), static_cast<int>(pTotalShares->getValue()), pTotalRealizedPL->getValue());
            getClientByUserID(pUserID->getValue())->receivePortfolioSummary();
        } catch (...) {
        }

        if (prevCnt) { // > 1 threads
            delete pTotalRealizedPL;
            delete pUserIDGroup;
            delete pUserID;
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
 * @param message as a QuoteAcknowledgement type object contains the current waiting list information.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::NewOrderList& message, const FIX::SessionID&) // override
{
    while (!m_connected)
        std::this_thread::sleep_for(10ms);

    static FIX::ClientBidID userID;
    static FIX::NoOrders n;

    static FIX50SP2::NewOrderList::NoOrders quoteSetGroup;
    static FIX::ClOrdID orderID;
    static FIX::Symbol originalName;
    static FIX::OrderQty size;
    static FIX::OrdType orderType;
    static FIX::Price price;
    static FIX::OrderQty2 executedSize;
    static FIX::PositionEffect status;

    // #pragma GCC diagnostic ignored ....

    FIX::ClientBidID* pUserID;
    FIX::NoOrders* pN;

    FIX50SP2::NewOrderList::NoOrders* pQuoteSetGroup;
    FIX::ClOrdID* pOrderID;
    FIX::Symbol* pOriginalName;
    FIX::OrderQty* pSize;
    FIX::OrdType* pOrderType;
    FIX::Price* pPrice;
    FIX::OrderQty2* pExecutedSize;
    FIX::PositionEffect* pStatus;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pUserID = &userID;
        pN = &n;
        pQuoteSetGroup = &quoteSetGroup;
        pOrderID = &orderID;
        pOriginalName = &originalName;
        pSize = &size;
        pOrderType = &orderType;
        pPrice = &price;
        pExecutedSize = &executedSize;
        pStatus = &status;
    } else { // > 1 threads; always safe way:
        pUserID = new decltype(userID);
        pN = new decltype(n);
        pQuoteSetGroup = new decltype(quoteSetGroup);
        pOrderID = new decltype(orderID);
        pOriginalName = new decltype(originalName);
        pSize = new decltype(size);
        pOrderType = new decltype(orderType);
        pPrice = new decltype(price);
        pExecutedSize = new decltype(executedSize);
        pStatus = new decltype(status);
    }

    message.getField(*pUserID);
    message.getField(*pN);

    std::vector<shift::Order> waitingList;

    for (int i = 1; i <= *pN; i++) {
        message.getGroup(static_cast<unsigned int>(i), *pQuoteSetGroup);

        pQuoteSetGroup->getField(*pOrderID);
        pQuoteSetGroup->getField(*pOriginalName);
        pQuoteSetGroup->getField(*pSize);
        pQuoteSetGroup->getField(*pOrderType);
        pQuoteSetGroup->getField(*pPrice);
        pQuoteSetGroup->getField(*pExecutedSize);
        pQuoteSetGroup->getField(*pStatus);

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
        getClientByUserID(pUserID->getValue())->storeWaitingList(std::move(waitingList));
        getClientByUserID(pUserID->getValue())->receiveWaitingList();
    } catch (...) {
    }

    if (prevCnt) { // > 1 threads
        delete pUserID;
        delete pN;
        delete pQuoteSetGroup;
        delete pOrderID;
        delete pOriginalName;
        delete pSize;
        delete pOrderType;
        delete pPrice;
        delete pExecutedSize;
        delete pStatus;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Method to get the open price of a certain symbol. Return the value from the open price book map.
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
 * @param symbol The symbol to be searched as a string.
 * @return The result last price as a double.
 */
double shift::FIXInitiator::getLastPrice(const std::string& symbol)
{
    return m_lastTrades[symbol].first;
}

/**
 * @brief Method to get the last traded size of a certain symbol. Return the value from the last trades map.
 * @param symbol The symbol to be searched as a string.
 * @return The result last size as an int.
 */
int shift::FIXInitiator::getLastSize(const std::string& symbol)
{
    return m_lastTrades[symbol].second;
}

/**
 * @brief Method to get the time of the last trade.
 * @return The result time of the last trade.
 */
std::chrono::system_clock::time_point shift::FIXInitiator::getLastTradeTime()
{
    return m_lastTradeTime;
}

/**
 * @brief Method to get the BestPrice object for the target symbol.
 * @param symbol The name of the symbol to be searched.
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
 * @param symbol The target symbol to find from the order book map.
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
 * @param symbol The target symbol to find from the order book map.
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
 * @return A vector contains all symbols for this session.
 */
std::vector<std::string> shift::FIXInitiator::getStockList()
{
    std::lock_guard<std::mutex> slGuard(m_mtxStockList);
    return m_stockList;
}

void shift::FIXInitiator::fetchCompanyName(std::string tickerName)
{
    // find the target url
    std::string modifiedName;
    std::regex nameTrimmer("\\.[^.]+$");

    if (tickerName.find(".")) {
        modifiedName = std::regex_replace(tickerName, nameTrimmer, "");
    } else {
        modifiedName = tickerName;
    }

    std::string url = "https://finance.yahoo.com/quote/" + modifiedName + "?p=" + modifiedName;

    // start using curl to crawl names down from yahoo
    CURL* curl;
    std::string html;
    std::string companyName;

    curl = curl_easy_init(); // initilise web query

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
        // since curl is a C library, need to cast url into c string to process, same for HTML
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITEFUNCTION_PTR>(sWriter)); // manages the required buffer size
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html); // data Pointer HTML stores downloaded web content
    } else {
        debugDump("Error creating curl object!");
    }

    if (curl_easy_perform(curl)) {
        debugDump("Error reading webpage!");
        curl_easy_cleanup(curl);
    } else {
        curl_easy_cleanup(curl); // close connection

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

    // it's ok to send repeated subscription requests:
    // a test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail
    m_subscribedOrderBookSet.insert(symbol);
    sendOrderBookRequest(m_symbol_originalName[symbol], true);
}

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
 * @brief Method to get the list of symbols who currently subscribed their order book.
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

    // it's ok to send repeated subscription requests:
    // a test to see if it is already included in the set here is bad because
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
 * @brief Method to get the list of symbols who currently subscribed their candle data.
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

#undef CSTR_BAD_USERID
