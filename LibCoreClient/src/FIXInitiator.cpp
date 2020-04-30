#include "FIXInitiator.h"

#include "CoreClient.h"
#include "Exceptions.h"
#include "OrderBookGlobalAsk.h"
#include "OrderBookGlobalBid.h"
#include "OrderBookLocalAsk.h"
#include "OrderBookLocalBid.h"
#include "Parameters.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <future>
#include <list>
#include <regex>
#include <thread>

#include <curl/curl.h>

#include <quickfix/FieldConvertors.h>
#include <quickfix/FieldTypes.h>

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

namespace shift {

/* static */ inline auto FIXInitiator::s_convertToTimePoint(const FIX::UtcDateOnly& date, const FIX::UtcTimeOnly& time) -> std::chrono::system_clock::time_point
{
    return std::chrono::system_clock::from_time_t(date.getTimeT()
        + time.getHour() * 3600
        + time.getMinute() * 60
        + time.getSecond());
}

/**
 * @brief Default constructor for FIXInitiator object.
 */
FIXInitiator::FIXInitiator()
    : m_connected { false }
    , m_verbose { false }
    , m_logonSuccess { false }
    , m_openPricesReady { false }
    , m_lastTradeTime { std::chrono::system_clock::time_point() }
{
}

/**
 * @brief Default destructor for FIXInitiator object.
 */
FIXInitiator::~FIXInitiator() // override
{
    {
        std::lock_guard<std::mutex> guard(m_mtxClientByUserID);
        m_clientByUserID.clear();
    }

    disconnectBrokerageCenter();
}

/**
 * @brief Get singleton instance of FIXInitiator.
 */
auto FIXInitiator::getInstance() -> FIXInitiator&
{
    static FIXInitiator fixInitiator;
    return fixInitiator;
}

auto FIXInitiator::connectBrokerageCenter(const std::string& configFile, CoreClient* client, const std::string& password, bool verbose /* = false */, int timeout /* = 10 */) -> bool
{
    disconnectBrokerageCenter();

    m_superUsername = client->getUsername();

    std::istringstream iss { password };
    crypto::Encryptor enc;
    iss >> enc >> m_superUserPsw;

    m_verbose = verbose;

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

    // See BrokerageCenter::FIXAcceptor::connectClients() for detailed explanation
    auto startTime = FIX::UtcTimeStamp();
    auto endTime = startTime;
    endTime += FIX_SESSION_DURATION;
    std::string startTimeStr = FIX::UtcTimeStampConvertor::convert(startTime);
    std::string endTimeStr = FIX::UtcTimeStampConvertor::convert(endTime);
    commonDict.setString("StartTime", startTimeStr.substr(startTimeStr.size() - 8));
    commonDict.setString("EndTime", endTimeStr.substr(endTimeStr.size() - 8));
    settings.set(commonDict);

    FIX::SessionID sessionID(commonDict.getString("BeginString"), toUpper(m_superUsername), commonDict.getString("TargetCompID"));
    FIX::Dictionary dict;
    settings.set(sessionID, std::move(dict));

    if (commonDict.has("FileLogPath")) { // store all log events into flat files
        m_pLogFactory = std::make_unique<FIX::FileLogFactory>(commonDict.getString("FileLogPath") + "/" + m_superUsername);
    } else { // display all log events onto the standard output
        m_pLogFactory = std::make_unique<FIX::ScreenLogFactory>(false, false, m_verbose); // incoming, outgoing, event
    }

    if (commonDict.has("FileStorePath")) { // store all outgoing messages into flat files
        m_pMessageStoreFactory = std::make_unique<FIX::FileStoreFactory>(commonDict.getString("FileStorePath") + "/" + m_superUsername);
    } else { // store all outgoing messages in memory
        m_pMessageStoreFactory = std::make_unique<FIX::MemoryStoreFactory>();
    }
    // else
    // { // do not store messages
    //     m_pMessageStoreFactory.reset(new FIX::NullStoreFactory());
    // }

    m_pSocketInitiator = std::make_unique<FIX::SocketInitiator>(*this, *m_pMessageStoreFactory, settings, *m_pLogFactory);

    try {
        m_pSocketInitiator->start();
    } catch (const FIX::ConfigError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        m_connected = false;
        return false;
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        m_connected = false;
        return false;
    }

    bool wasNotified = false;
    {
        std::unique_lock<std::mutex> lsUniqueLock(m_mtxLogon);
        auto t1 = std::chrono::steady_clock::now();
        // gets notify from fromAdmin() or onLogon(), or timeout if backend cannot resolve (e.g. no such user in DB)
        m_cvLogon.wait_for(lsUniqueLock, timeout * 1s);
        auto t2 = std::chrono::steady_clock::now();

        wasNotified = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count() < timeout;
    }

    if (m_logonSuccess) {
        std::unique_lock<std::mutex> slUniqueLock(m_mtxStockList);
        // gets notify when security list is ready
        m_cvStockList.wait_for(slUniqueLock, timeout * 1s);

        if (attachClient(client)) { // attach super user and register in BrokerageCenter
            m_superUserID = client->getUserID();
        } else {
            m_connected = false;
            return false;
        }

        // register already attached client users in BrokerageCenter
        for (const auto& c : getAttachedClients()) {
            if (!registerUserInBCWaitResponse(c)) {
                cout << COLOR_ERROR << "Failed to register client [" << c->getUsername() << "] in Brokerage Center." NO_COLOR << endl;
                std::terminate(); // precondition broken: attached clients (by attachClient()) cannot fail registering
            }
        }
    } else {
        disconnectBrokerageCenter();

        if (wasNotified) {
            throw IncorrectPasswordError();
        }

        throw ConnectionTimeoutError();
    }

    m_connected = true;
    return true;
}

void FIXInitiator::disconnectBrokerageCenter()
{
    // stop QuickFIX
    if (m_pSocketInitiator) {
        m_pSocketInitiator->stop();
        m_pSocketInitiator = nullptr;
    }
    m_pMessageStoreFactory = nullptr;
    m_pLogFactory = nullptr;

    // house cleaning:

    // cancel all pending client requests:
    // (we force a disconnect before every connection - even the first one)
    if (m_connected) {
        getSuperUser()->cancelAllSamplePricesRequests();
        for (auto& client : getAttachedClients()) {
            client->cancelAllSamplePricesRequests();
        }
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
        std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandlestickDataSet);
        m_subscribedCandlestickDataSet.clear();
    }
}

auto FIXInitiator::attachClient(CoreClient* client, const std::string& password /* = "NA" */) -> bool
{
    // TODO: add auth in BC; use password to auth

    if (client == nullptr) {
        return false;
    }

    if (!registerUserInBCWaitResponse(client)) {
        cout << COLOR_ERROR << "Failed to register client [" << client->getUsername() << "] in Brokerage Center." NO_COLOR << endl;
        cout << COLOR_ERROR << "Registering will be reattempted upon successful connection." NO_COLOR << endl;
    }

    {
        std::lock_guard<std::mutex> guard(m_mtxClientByUserID);
        m_clientByUserID[client->getUserID()] = client;
    }

    return client->attachInitiator(*this);
}

auto FIXInitiator::registerUserInBCWaitResponse(CoreClient* client) -> bool
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserRequest));

    message.setField(FIX::UserRequestID(crossguid::newGuid().str()));
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

auto FIXInitiator::getAttachedClients() -> std::vector<CoreClient*>
{
    std::vector<CoreClient*> clientsVector;

    std::lock_guard<std::mutex> guard(m_mtxClientByUserID);

    for (const auto& [userID, client] : m_clientByUserID) {
        if (userID != m_superUserID) { // skip super user
            clientsVector.push_back(client);
        }
    }

    return clientsVector;
}

auto FIXInitiator::getSuperUser() -> CoreClient*
{
    return getClientByUserID(m_superUserID);
}

auto FIXInitiator::getClient(const std::string& username) -> CoreClient*
{
    std::string userID;
    {
        std::lock_guard<std::mutex> guard(m_mtxUserIDByUsername);
        userID = m_userIDByUsername[username];
    }

    return getClientByUserID(userID);
}

auto FIXInitiator::getClientByUserID(const std::string& userID) -> CoreClient*
{
    std::lock_guard<std::mutex> guard(m_mtxClientByUserID);

    if (m_clientByUserID.find(userID) == m_clientByUserID.end()) {
        throw std::range_error("User not found: " + userID);
    }

    return m_clientByUserID[userID];
}

auto FIXInitiator::isConnected() const -> bool
{
    return m_connected;
}

/**
 * @brief Method to print log messages.
 * @param message: a string to be print.
 */
inline void FIXInitiator::debugDump(const std::string& message) const
{
    if (m_verbose) {
        cout << "FIXInitiator:" << endl;
        cout << message << endl;
    }
}

/**
 * @brief Method to create internal symbols mapping (e.g. AAPL.O -> AAPL).
 */
inline void FIXInitiator::createSymbolMap()
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
inline void FIXInitiator::initializePrices()
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
inline void FIXInitiator::initializeOrderBooks()
{
    for (const auto& symbol : m_stockList) {
        m_orderBooks[symbol][OrderBook::Type::GLOBAL_ASK] = std::make_unique<OrderBookGlobalAsk>(symbol);
        m_orderBooks[symbol][OrderBook::Type::GLOBAL_BID] = std::make_unique<OrderBookGlobalBid>(symbol);
        m_orderBooks[symbol][OrderBook::Type::LOCAL_ASK] = std::make_unique<OrderBookLocalAsk>(symbol);
        m_orderBooks[symbol][OrderBook::Type::LOCAL_BID] = std::make_unique<OrderBookLocalBid>(symbol);
    }
}

/*
 * @brief send order book request to BC.
 */
/* static */ void FIXInitiator::s_sendOrderBookRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(crossguid::newGuid().str()));
    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SUBSCRIBE);
        message.setField(::FIXFIELD_MDUPDATETYPE_INCREMENTAL_REFRESH);
    } else {
        message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_UNSUBSCRIBE);
    }
    message.setField(::FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH); // required by FIX

    fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message,
        ::FIXFIELD_MDENTRYTYPE_BID);
    fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message,
        ::FIXFIELD_MDENTRYTYPE_OFFER);
    fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoRelatedSym>(message,
        FIX::Symbol(symbol));

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send candle data request to BC.
 */
/* static */ void FIXInitiator::s_sendCandlestickDataRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_RFQRequest));

    message.setField(FIX::RFQReqID(crossguid::newGuid().str()));

    fix::addFIXGroup<FIX50SP2::RFQRequest::NoRelatedSym>(message,
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
 * @param order as a Order object contains all required information.
 * @param userID as identifier for the user/client who is submitting the order.
 */
void FIXInitiator::submitOrder(const Order& order, const std::string& userID /* = "" */)
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

    fix::addFIXGroup<FIX50SP2::NewOrderSingle::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        !userID.empty() ? FIX::PartyID(userID) : FIX::PartyID(m_superUserID));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method called when a new Session was created. Set Sender and Target Comp ID.
 */
void FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
    debugDump("SenderID: " + s_senderID + "\nTargetID: " + s_targetID);
}

/**
 * @brief Method called when user logon to the system through FIX session. Create a Logon record in the execution log.
 */
void FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogon - " + sessionID.toString());

    m_logonSuccess = true;
    m_cvLogon.notify_one();
}

/**
 * @brief Method called when user log out from the system. Create a Logon record in the execution log.
 */
void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    debugDump("\nLogout - " + sessionID.toString());
}

/**
 * @brief This Method was called when user is trying to login. It sends the username and password information to Admin for verification.
 */
void FIXInitiator::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) // override
{
    if (FIX::MsgType_Logon == message.getHeader().getField(FIX::FIELD::MsgType)) {
        fix::addFIXGroup<FIXT11::Logon::NoMsgTypes>(message,
            FIX::RefMsgType(m_superUsername));
        fix::addFIXGroup<FIXT11::Logon::NoMsgTypes>(message,
            FIX::RefMsgType(m_superUserPsw));
    }
}

/**
 * @brief Method for passing message to Core Client.
 */
void FIXInitiator::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) noexcept(false) // override
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
void FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) noexcept(false) // override
{
    crack(message, sessionID);
}

/**
 * @brief Receive the security list from BrokerageCenter.
 */
void FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    if (m_stockList.empty()) {
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
void FIXInitiator::onMessage(const FIX50SP2::UserResponse& message, const FIX::SessionID& sessionID) // override
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
void FIXInitiator::onMessage(const FIX50SP2::Advertisement& message, const FIX::SessionID& sessionID) // override
{
    if (m_connected) {
        static FIX::Symbol originalName;
        static FIX::Quantity size;
        static FIX::Price price;
        static FIX::TransactTime simulationTime;
        static FIX::LastMkt destination;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pOriginalName = nullptr;
        FIX::Quantity* pSize = nullptr;
        FIX::Price* pPrice = nullptr;
        FIX::TransactTime* pSimulationTime = nullptr;
        FIX::LastMkt* pDestination = nullptr;

        static std::atomic<unsigned int> s_cntAtom { 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
        }

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

        if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh& message, const FIX::SessionID& sessionID) // override
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

    FIX::Symbol* pOriginalName = nullptr;

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries* pEntryGroup = nullptr;
    FIX::MDEntryType* pBookType = nullptr;
    FIX::MDEntryPx* pPrice = nullptr;
    FIX::MDEntrySize* pSize = nullptr;
    FIX::MDEntryDate* pSimulationDate = nullptr;
    FIX::MDEntryTime* pSimulationTime = nullptr;
    FIX::MDMkt* pDestination = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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
    std::list<OrderBookEntry> orderBook;

    for (int i = 1; i <= numOfEntries.getValue(); ++i) {
        message.getGroup(static_cast<unsigned int>(i), *pEntryGroup);

        pEntryGroup->getField(*pBookType);
        pEntryGroup->getField(*pPrice);
        pEntryGroup->getField(*pSize);
        pEntryGroup->getField(*pSimulationDate);
        pEntryGroup->getField(*pSimulationTime);
        pEntryGroup->getField(*pDestination);

        orderBook.emplace_back(static_cast<double>(pPrice->getValue()),
            static_cast<int>(pSize->getValue()),
            pDestination->getValue(),
            s_convertToTimePoint(pSimulationDate->getValue(), pSimulationTime->getValue()));
    }

    m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->setOrderBook(std::move(orderBook));

    if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID& sessionID) // override
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

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries* pEntryGroup = nullptr;
    FIX::MDEntryType* pBookType = nullptr;
    FIX::Symbol* pOriginalName = nullptr;
    FIX::MDEntryPx* pPrice = nullptr;
    FIX::MDEntrySize* pSize = nullptr;
    FIX::MDEntryDate* pSimulationDate = nullptr;
    FIX::MDEntryTime* pSimulationTime = nullptr;
    FIX::MDMkt* pDestination = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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
        OrderBookEntry entry {
            pPrice->getValue(),
            static_cast<int>(pSize->getValue()),
            pDestination->getValue(),
            s_convertToTimePoint(pSimulationDate->getValue(), pSimulationTime->getValue())
        };
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->update(std::move(entry));
    } else {
        m_orderBooks[symbol][static_cast<OrderBook::Type>(pBookType->getValue())]->resetOrderBook();
    }

    if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID& sessionID) // override
{
    static FIX::Symbol originalName;
    static FIX::HighPx highPrice;
    static FIX::LowPx lowPrice;
    static FIX::LastPx closePrice;
    static FIX::FirstPx openPrice;
    static FIX::TransactTime timestamp;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pOriginalName = nullptr;
    FIX::HighPx* pHighPrice = nullptr;
    FIX::LowPx* pLowPrice = nullptr;
    FIX::LastPx* pClosePrice = nullptr;
    FIX::FirstPx* pOpenPrice = nullptr;
    FIX::TransactTime* pTimestamp = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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

    if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) // override
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

    FIX::OrderID* pOrderID = nullptr;
    FIX::OrdStatus* pStatus = nullptr;
    FIX::OrdType* pOrderType = nullptr;
    FIX::Price* pExecutedPrice = nullptr;
    FIX::CumQty* pExecutedSize = nullptr;

    FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup = nullptr;
    FIX::PartyID* pUserID = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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
        getClientByUserID(pUserID->getValue())->storeExecution(pOrderID->getValue(), static_cast<Order::Type>(pOrderType->getValue()), static_cast<int>(pExecutedSize->getValue()), pExecutedPrice->getValue(), static_cast<Order::Status>(pStatus->getValue()));
        getClientByUserID(pUserID->getValue())->receiveExecution(pOrderID->getValue());
    } catch (...) {
    }

    if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::PositionReport& message, const FIX::SessionID& sessionID) // override
{
    while (!m_connected) {
        std::this_thread::sleep_for(10ms);
    }

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

        FIX::Symbol* pOriginalName = nullptr;
        FIX::SettlPrice* pLongPrice = nullptr;
        FIX::PriorSettlPrice* pShortPrice = nullptr;
        FIX::PriceDelta* pRealizedPL = nullptr;

        FIX50SP2::PositionReport::NoPartyIDs* pUserIDGroup = nullptr;
        FIX::PartyID* pUserID = nullptr;

        FIX50SP2::PositionReport::NoPositions* pSizeGroup = nullptr;
        FIX::LongQty* pLongSize = nullptr;
        FIX::ShortQty* pShortSize = nullptr;

        static std::atomic<unsigned int> s_cntAtom { 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
        }

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

        if (0 != prevCnt) { // > 1 threads
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

        FIX::PriceDelta* pTotalRealizedPL = nullptr;

        FIX50SP2::PositionReport::NoPartyIDs* pUserIDGroup = nullptr;
        FIX::PartyID* pUserID = nullptr;

        FIX50SP2::PositionReport::NoPositions* pTotalSharesGroup = nullptr;
        FIX::LongQty* pTotalShares = nullptr;

        FIX50SP2::PositionReport::NoPosAmt* pTotalBuyingPowerGroup = nullptr;
        FIX::PosAmt* pTotalBuyingPower = nullptr;

        static std::atomic<unsigned int> s_cntAtom { 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
        }

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

        if (0 != prevCnt) { // > 1 threads
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
void FIXInitiator::onMessage(const FIX50SP2::NewOrderList& message, const FIX::SessionID& sessionID) // override
{
    while (!m_connected) {
        std::this_thread::sleep_for(10ms);
    }

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

    FIX::ClientBidID* pUserID = nullptr;
    FIX::NoOrders* pN = nullptr;

    FIX50SP2::NewOrderList::NoOrders* pQuoteSetGroup = nullptr;
    FIX::ClOrdID* pOrderID = nullptr;
    FIX::Symbol* pOriginalName = nullptr;
    FIX::OrderQty* pSize = nullptr;
    FIX::OrdType* pOrderType = nullptr;
    FIX::Price* pPrice = nullptr;
    FIX::OrderQty2* pExecutedSize = nullptr;
    FIX::PositionEffect* pStatus = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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

    std::vector<Order> waitingList;

    for (int i = 1; i <= *pN; ++i) {
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

            Order order {
                static_cast<Order::Type>(pOrderType->getValue()),
                symbol,
                sizeInt,
                pPrice->getValue(),
                pOrderID->getValue()
            };
            order.setExecutedSize(static_cast<int>(pExecutedSize->getValue()));
            order.setStatus(static_cast<Order::Status>(pStatus->getValue()));

            waitingList.push_back(std::move(order));
        }
    }

    try {
        getClientByUserID(pUserID->getValue())->storeWaitingList(std::move(waitingList));
        getClientByUserID(pUserID->getValue())->receiveWaitingList();
    } catch (...) {
    }

    if (0 != prevCnt) { // > 1 threads
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
auto FIXInitiator::getOpenPrice(const std::string& symbol) -> double
{
    std::lock_guard<std::mutex> opGuard(m_mtxOpenPrices);

    if (m_openPrices.find(symbol) == m_openPrices.end()) {
        return 0.0;
    }

    return m_openPrices[symbol];
}

/**
 * @brief Method to get the last traded price of a certain symbol. Return the value from the last trades map.
 * @param symbol The symbol to be searched as a string.
 * @return The result last price as a double.
 */
auto FIXInitiator::getLastPrice(const std::string& symbol) -> double
{
    return m_lastTrades[symbol].first;
}

/**
 * @brief Method to get the last traded size of a certain symbol. Return the value from the last trades map.
 * @param symbol The symbol to be searched as a string.
 * @return The result last size as an int.
 */
auto FIXInitiator::getLastSize(const std::string& symbol) -> int
{
    return m_lastTrades[symbol].second;
}

/**
 * @brief Method to get the time of the last trade.
 * @return The result time of the last trade.
 */
auto FIXInitiator::getLastTradeTime() -> std::chrono::system_clock::time_point
{
    return m_lastTradeTime;
}

/**
 * @brief Method to get the BestPrice object for the target symbol.
 * @param symbol The name of the symbol to be searched.
 * @return BestPrice for the target symbol.
 */
auto FIXInitiator::getBestPrice(const std::string& symbol) -> BestPrice
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Best Price for symbol " + symbol;
    }

    auto globalBidBestValues = std::async(std::launch::async, &OrderBook::getBestValues, m_orderBooks[symbol][OrderBook::Type::GLOBAL_BID].get());
    auto globalAskBestValues = std::async(std::launch::async, &OrderBook::getBestValues, m_orderBooks[symbol][OrderBook::Type::GLOBAL_ASK].get());
    auto localBidBestValues = std::async(std::launch::async, &OrderBook::getBestValues, m_orderBooks[symbol][OrderBook::Type::LOCAL_BID].get());
    auto localAskBestValues = std::async(std::launch::async, &OrderBook::getBestValues, m_orderBooks[symbol][OrderBook::Type::LOCAL_ASK].get());

    return { globalBidBestValues.get(), globalAskBestValues.get(),
        localBidBestValues.get(), localAskBestValues.get() };
}

/**
 * @brief Method to get the corresponding order book by symbol name and entry type.
 * @param symbol The target symbol to find from the order book map.
 * @param type The target entry type (GLOBAL_BID, GLOBAL_ASK, LOCAL_BID, LOCAL_ASK)
 */
auto FIXInitiator::getOrderBook(const std::string& symbol, OrderBook::Type type, int maxLevel) -> std::vector<OrderBookEntry>
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Order Book for symbol " + symbol;
    }

    // this test is not necessary anymore since using an enum OrderBook::Type
    // if (m_orderBooks[symbol].find(type) == m_orderBooks[symbol].end()) {
    //     throw "Order Book type is invalid";
    // }

    return m_orderBooks[symbol][type]->getOrderBook(maxLevel);
}

/**
 * @brief Method to get the corresponding order book with destination by symbol name and entry type.
 * @param symbol The target symbol to find from the order book map.
 * @param type The target entry type (GLOBAL_BID, GLOBAL_ASK, LOCAL_BID, LOCAL_ASK)
 */
auto FIXInitiator::getOrderBookWithDestination(const std::string& symbol, OrderBook::Type type) -> std::vector<OrderBookEntry>
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Order Book for symbol " + symbol;
    }

    // this test is not necessary anymore since using an enum OrderBook::Type
    // if (m_orderBooks[symbol].find(type) == m_orderBooks[symbol].end()) {
    //     throw "Order Book type is invalid";
    // }

    return m_orderBooks[symbol][type]->getOrderBookWithDestination();
}

/**
 * @brief Method to get the current stock list.
 * @return A vector contains all symbols for this session.
 */
auto FIXInitiator::getStockList() -> std::vector<std::string>
{
    std::lock_guard<std::mutex> slGuard(m_mtxStockList);
    return m_stockList;
}

void FIXInitiator::fetchCompanyName(std::string tickerName)
{
    // find the target url
    std::string modifiedName;
    std::regex nameTrimmer("\\.[^.]+$");

    if (tickerName.find(".") != 0) {
        modifiedName = std::regex_replace(tickerName, nameTrimmer, "");
    } else {
        modifiedName = tickerName;
    }

    std::string url = "https://finance.yahoo.com/quote/" + modifiedName + "?p=" + modifiedName;

    // start using curl to crawl names down from yahoo
    CURL* curl = nullptr;
    std::string html;
    std::string companyName;

    curl = curl_easy_init(); // initilise web query

    if (curl != nullptr) {
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

    if (curl_easy_perform(curl) != 0) {
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

void FIXInitiator::requestCompanyNames()
{
    for (const auto& symbol : getStockList()) {
        std::thread(&FIXInitiator::fetchCompanyName, this, symbol).detach();
    }
}

auto FIXInitiator::getCompanyNames() -> std::map<std::string, std::string>
{
    std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
    return m_companyNames;
}

auto FIXInitiator::getCompanyName(const std::string& symbol) -> std::string
{
    std::lock_guard<std::mutex> cnGuard(m_mtxCompanyNames);
    return m_companyNames[symbol];
}

void FIXInitiator::subOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    // WARNING: the following is not a requirement anymore
    // it's ok to send repeated subscription requests:
    // a test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail
    m_subscribedOrderBookSet.insert(symbol);
    s_sendOrderBookRequest(m_symbol_originalName[symbol], true);
}

void FIXInitiator::unsubOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    if (m_subscribedOrderBookSet.find(symbol) != m_subscribedOrderBookSet.end()) {
        m_subscribedOrderBookSet.erase(symbol);
        s_sendOrderBookRequest(m_symbol_originalName[symbol], false);
    }
}

void FIXInitiator::subAllOrderBook()
{
    for (const auto& symbol : getStockList()) {
        subOrderBook(symbol);
    }
}

void FIXInitiator::unsubAllOrderBook()
{
    for (const auto& symbol : getStockList()) {
        unsubOrderBook(symbol);
    }
}

/**
 * @brief Method to get the list of symbols who currently subscribed their order book.
 * @return  A vector of symbols who subscribed their order book.
 */
auto FIXInitiator::getSubscribedOrderBookList() -> std::vector<std::string>
{
    std::lock_guard<std::mutex> soblGuard(m_mtxSubscribedOrderBookSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedOrderBookSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}

void FIXInitiator::subCandlestickData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandlestickDataSet);

    // WARNING: the following is not a requirement anymore
    // it's ok to send repeated subscription requests:
    // a test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail.
    m_subscribedCandlestickDataSet.insert(symbol);
    s_sendCandlestickDataRequest(m_symbol_originalName[symbol], true);
}

void FIXInitiator::unsubCandlestickData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandlestickDataSet);

    if (m_subscribedCandlestickDataSet.find(symbol) != m_subscribedCandlestickDataSet.end()) {
        m_subscribedCandlestickDataSet.erase(symbol);
        s_sendCandlestickDataRequest(m_symbol_originalName[symbol], false);
    }
}

void FIXInitiator::subAllCandlestickData()
{
    for (const auto& symbol : getStockList()) {
        subCandlestickData(symbol);
    }
}

void FIXInitiator::unsubAllCandlestickData()
{
    for (const auto& symbol : getStockList()) {
        unsubCandlestickData(symbol);
    }
}

/**
 * @brief Method to get the list of symbols who currently subscribed their candle data.
 * @return A vector of symbols who subscribed their candle data.
 */
auto FIXInitiator::getSubscribedCandlestickDataList() -> std::vector<std::string>
{
    std::lock_guard<std::mutex> scslGuard(m_mtxSubscribedCandlestickDataSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedCandlestickDataSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}

} // shift

#undef CSTR_BAD_USERID
