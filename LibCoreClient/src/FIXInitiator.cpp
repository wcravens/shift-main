#include "FIXInitiator.h"

#include "CoreClient.h"
#include "Exceptions.h"
#include "OrderBookGlobalAsk.h"
#include "OrderBookGlobalBid.h"
#include "OrderBookLocalAsk.h"
#include "OrderBookLocalBid.h"

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

// Predefined constant FIX fields (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1");
static const auto& FIXFIELD_UQ_ACTION = FIX::UserRequestType(1); // Action required by a UserRequest, 1 = Log On User
static const auto& FIXFIELD_SUBTYPE_SUB = FIX::SubscriptionRequestType('1'); // Subscription Request Type, 1 = Snapshot + Updates (Subscribe)
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType('F'); // F = Trade
static const auto& FIXFIELD_SUBTYPE_UNSUB = FIX::SubscriptionRequestType('2'); // Subscription Request Type, 2 = Disable previous Snapshot + Update Request (Unsubscribe)
static const auto& FIXFIELD_UPDTYPE_INCRE = FIX::MDUpdateType(1); // Type of Market Data update, 1 = Incremental refresh
static const auto& FIXFIELD_DOM_FULL = FIX::MarketDepth(0); // Depth of market for Book Snapshot, 0 = full book depth
static const auto& FIXFIELD_ENTRY_BID = FIX::MDEntryType('0'); // Type of market data entry, 0 = Bid
static const auto& FIXFIELD_ENTRY_OFFER = FIX::MDEntryType('1'); // Type of market data entry, 1 = Offer
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = ClientID in FIX4.2

inline std::chrono::system_clock::time_point shift::FIXInitiator::s_convertToTimePoint(const FIX::UtcDateOnly& date, const FIX::UtcTimeOnly& time)
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
        std::lock_guard<std::mutex> ucGuard(m_mutexUsernameClient);
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

void shift::FIXInitiator::connectBrokerageCenter(const std::string& cfgFile, CoreClient* client, const std::string& passwd, bool verbose, int timeout)
{
    disconnectBrokerageCenter();

    m_username = client->getUsername();

    std::istringstream iss{ passwd };
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
    std::unique_lock<std::mutex> lsUniqueLock(m_mutexLogon);
    // Gets notify from fromAdmin() or onLogon()
    m_cvLogon.wait_for(lsUniqueLock, timeout * 1ms);
    auto t2 = std::chrono::steady_clock::now();
    timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    if (m_logonSuccess) {
        std::unique_lock<std::mutex> slUniqueLock(m_mutexStockList);
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
        std::lock_guard<std::mutex> slGuard(m_mutexStockList);
        m_stockList.clear();
        m_originalNameSymbol.clear();
        m_symbolOriginalName.clear();
    }
    {
        std::lock_guard<std::mutex> cnGuard(m_mutexCompanyNames);
        m_companyNames.clear();
    }

    // Subscriptions will reset automatically when disconnecting,
    // but we still need to clean up these sets
    {
        std::lock_guard<std::mutex> soblGuard(m_mutexSubscribedOrderBookSet);
        m_subscribedOrderBookSet.clear();
    }
    {
        std::lock_guard<std::mutex> scslGuard(m_mutexSubscribedCandleStickSet);
        m_subscribedCandleStickSet.clear();
    }
}

/**
 * @section WARNING
 * Don't call this function directly
 */
void shift::FIXInitiator::attach(shift::CoreClient* c, const std::string& password, int timeout)
{
    // TODO: add auth in BC and add lock after successful logon; use password to auth
    webClientSendUsername(c->getUsername());

    {
        std::lock_guard<std::mutex> ucGuard(m_mutexUsernameClient);
        m_usernameClientMap[c->getUsername()] = c;
    }

    c->attach(*this);
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
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserRequest));

    message.setField(FIX::UserRequestID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_UQ_ACTION); // Required by FIX
    message.setField(FIX::Username(username));

    FIX::Session::sendToTarget(message);
}

std::vector<shift::CoreClient*> shift::FIXInitiator::getAttachedClients()
{
    std::vector<shift::CoreClient*> clientsVector;

    std::lock_guard<std::mutex> ucGuard(m_mutexUsernameClient);

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
    std::lock_guard<std::mutex> ucGuard(m_mutexUsernameClient);

    if (m_usernameClientMap.find(name) != m_usernameClientMap.end()) {
        return m_usernameClientMap[name];
    } else {
        throw std::range_error("Name not found: " + name);
    }
}

bool shift::FIXInitiator::isConnected()
{
    return m_connected;
}

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
        m_originalNameSymbol[originalName] = originalName.substr(0, originalName.find_last_of('.'));
        m_symbolOriginalName[m_originalNameSymbol[originalName]] = originalName;
        // Substitute the old name with new symbol
        originalName = m_originalNameSymbol[originalName];
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

    std::lock_guard<std::mutex> opGuard(m_mutexOpenPrices);
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

/**
 * @brief Method called when a new Session was created.
 * Set Sender and Target Comp ID.
 */
void shift::FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID();
    s_targetID = sessionID.getTargetCompID();
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
 * @brief Method to receive Last Price from
 * Brokerage Center.
 *
 * @param message as a Advertisement type object
 * contains the last price information.
 *
 * @param sessionID as the SessionID for the current
 * communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::Advertisement& message, const FIX::SessionID&) // override
{
    if (m_connected) {
        FIX::Symbol symbol;
        FIX::Quantity size;
        FIX::Price price;
        FIX::TransactTime simulationTime;
        FIX::LastMkt destination;

        message.get(symbol);
        message.get(size);
        message.get(price);
        message.get(simulationTime);
        message.get(destination);

        symbol = m_originalNameSymbol[symbol];

        m_lastTrades[symbol].first = price;
        m_lastTrades[symbol].second = int(size);
        m_lastTradeTime = std::chrono::system_clock::from_time_t(simulationTime.getValue().getTimeT());
        try {
            getMainClient()->receiveLastPrice(symbol);
        } catch (...) {
            return;
        }

        return;
    }
}

/**
 * @brief Method to receive LatestStockPrice from
 * Brokerage Center.
 *
 * @param message as a ExecutionReport type object
 * contains the last price information.
 *
 * @param sessionID as the SessionID for the current
 * communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) // override
{
    // FIX::OrdStatus orderStatus;
    // message.get(orderStatus);
    // if (orderStatus == '1') {
    //     FIX::Symbol symbol;
    //     message.get(symbol);
    //     FIX::OrderID orderID;
    //     message.get(orderID);
    //     FIX::ClientID clientID;
    //     message.get(clientID);
    //     FIX::CumQty size;
    //     message.get(size);
    //     FIX::AvgPx price;
    //     message.get(price);
    //     FIX::ExecType orderType;
    //     message.get(orderType);
    //     FIX::Text serverTime;
    //     message.get(serverTime);
    //     FIX::ExecID confirmTime;
    //     message.get(confirmTime);

    //     // shift::Report report(clientID, orderID, symbol, price, orderType, orderStatus, size,
    //     //                      confirmTime, serverTime);
    // } else {
    //     FIX::Symbol symbol;
    //     message.get(symbol);
    //     FIX::ClientID clientID1;
    //     message.get(clientID1);
    //     FIX::OrderID orderID1;
    //     message.get(orderID1);
    //     FIX::ExecType orderType1;
    //     message.get(orderType1);
    //     FIX::ClOrdID clientID2;
    //     message.get(clientID2);
    //     FIX::SecondaryOrderID orderID2;
    //     message.get(orderID2);
    //     FIX::SettlmntTyp orderType2;
    //     message.get(orderType2);
    //     FIX::CumQty size;
    //     message.get(size);
    //     FIX::AvgPx price;
    //     message.get(price);
    //     FIX::LeavesQty leftQuantity;
    //     message.get(leftQuantity);
    //     FIX::Text serverTime;
    //     message.get(serverTime);
    //     FIX::ExecID executionTime;
    //     message.get(executionTime);
    //     FIX::ExecBroker destination;
    //     message.get(destination);
    // }
}

/**
 * @brief Method to receive portfolio item and summary from Brokerage Center.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::PositionReport& message, const FIX::SessionID&) // override
{
    while (!m_connected)
        ;

    FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
    FIX::PartyID username;
    message.getGroup(1, usernameGroup);
    usernameGroup.get(username);

    FIX::SecurityType type;
    message.get(type);

    if (type == "CS") { // Item
        FIX::Symbol symbol;
        message.get(symbol);

        // m_originalName_symbol shall be always thread-safe-readonly once after being initialized, so we shall prevent it from accidental insertion here
        if (m_originalNameSymbol.find(symbol) == m_originalNameSymbol.end()) {
            cout << COLOR_WARNING "FIX50SP2::PositionReport received an unknown symbol [" << symbol << "], skipped." NO_COLOR << endl;
            return; //
        }

        symbol = m_originalNameSymbol[symbol];

        FIX::SettlPrice longPrice;
        FIX::PriorSettlPrice shortPrice;
        FIX::PriceDelta realizedPL;
        FIX::LongQty longQty;
        FIX::ShortQty shortQty;

        message.get(longPrice);
        message.get(shortPrice);
        message.get(realizedPL);

        FIX50SP2::PositionReport::NoPositions qtyGroup;
        message.getGroup(1, qtyGroup);
        qtyGroup.get(longQty);
        qtyGroup.get(shortQty);

        try {
            getClient(username)->storePortfolioItem(symbol, static_cast<int>(longQty), static_cast<int>(shortQty), longPrice, shortPrice, realizedPL);
            getClient(username)->receivePortfolioItem(symbol);
        } catch (...) {
            return;
        }
    } else { // Summary
        FIX::PriceDelta totalRealizedPL;
        FIX::LongQty totalShares;
        FIX::PosAmt totalBuyingPower;

        message.get(totalRealizedPL);

        FIX50SP2::PositionReport::NoPositions qtyGroup;
        FIX50SP2::PositionReport::NoPosAmt buyingPowerGroup;

        message.getGroup(1, qtyGroup);
        qtyGroup.get(totalShares);

        message.getGroup(1, buyingPowerGroup);
        buyingPowerGroup.get(totalBuyingPower);

        try {
            getClient(username)->storePortfolioSummary(totalBuyingPower, static_cast<int>(totalShares), totalRealizedPL);
            getClient(username)->receivePortfolioSummary();
        } catch (...) {
            return;
        }
    }
}

/**
 * @brief Method to receive Global/Local orders from Brokerage Center.
 *
 * @param message as a MarketDataSnapshotFullRefresh type object contains the current accepting order information.
 *
 * @param sessionID as the SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntry;
    message.getField(numOfEntry);
    if (!numOfEntry) {
        cout << "Cannot find MDEntry in MarketDataSnapshotFullRefresh!" << endl;
        return;
    }

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries::NoPartyIDs partyGroup;

    FIX::Symbol symbol;
    FIX::MDEntryType type;
    FIX::MDEntryPx price;
    FIX::MDEntrySize size;
    FIX::MDEntryDate date;
    FIX::MDEntryTime daytime;

    FIX::NoPartyIDs numOfParty;
    FIX::PartyID destination;

    std::list<shift::OrderBookEntry> orderBook;

    message.getField(symbol);
    symbol = m_originalNameSymbol[symbol];

    for (int i = 1; i <= numOfEntry; i++) {
        message.getGroup(static_cast<unsigned int>(i), entryGroup);

        entryGroup.getField(type);
        entryGroup.getField(price);
        entryGroup.getField(size);
        entryGroup.getField(date);
        entryGroup.getField(daytime);

        entryGroup.getField(numOfParty);
        if (!numOfParty) {
            cout << "Cannot find PartyID in NoMDEntries!" << endl;
            return;
        }

        entryGroup.getGroup(1, partyGroup);
        partyGroup.getField(destination);

        orderBook.push_back({ static_cast<double>(price),
            int(size),
            destination,
            s_convertToTimePoint(date.getValue(), daytime.getValue()) });
    }

    try {
        m_orderBooks[symbol][static_cast<OrderBook::Type>(static_cast<char>(type))]->setOrderBook(orderBook);
    } catch (std::exception e) {
        debugDump(symbol.getString() + " doesn't work");
    }
}

/**
 * @brief Method to receive portfolio update from Brokerage Center.
 *
 * @param message as a QuoteAcknowledgement type object contains the current accepting order information.
 *
 * @param sessionID as the SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID&) // override
{
    FIX::Account username;
    message.get(username);

    while (!m_connected)
        ;

    FIX::NoQuoteSets n;
    message.get(n);

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;

    FIX::QuoteSetID userID;
    FIX::UnderlyingSymbol symbol;
    FIX::UnderlyingSymbolSfx orderSize;
    FIX::UnderlyingSecurityID orderID;
    FIX::UnderlyingStrikePrice price;
    FIX::UnderlyingIssuer orderType;

    std::vector<shift::Order> waitingList;

    for (int i = 1; i <= n; i++) {
        message.getGroup(static_cast<unsigned int>(i), quoteSetGroup);

        quoteSetGroup.get(userID);
        quoteSetGroup.get(symbol);
        quoteSetGroup.get(orderSize);
        quoteSetGroup.get(orderID);
        quoteSetGroup.get(price);
        quoteSetGroup.get(orderType);

        symbol = m_originalNameSymbol[symbol];
        int size = atoi((std::string(orderSize)).c_str());
        shift::Order::Type type = shift::Order::Type(char(atoi(orderType.getString().c_str())));

        if (size != 0) {
            waitingList.push_back({ type, symbol, size, price, orderID });
        }
    }

    try {
        // store the data in target client
        getClient(username)->storeWaitingList(std::move(waitingList));
        // notify target client
        getClient(username)->receiveWaitingList();
    } catch (...) {
        return;
    }
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
    FIX::NoMDEntries numOfEntry;

    message.get(numOfEntry);
    if (numOfEntry < 1) {
        cout << "Cannot find the entry group in message !" << endl;
        return;
    }

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;
    message.getGroup(1, entryGroup);
    entryGroup.getGroup(1, partyGroup);

    FIX::MDEntryType type;
    FIX::Symbol symbol;
    FIX::MDEntryPx price;
    FIX::MDEntrySize size;
    FIX::PartyID destination;
    FIX::MDEntryDate date;
    FIX::MDEntryTime daytime;

    entryGroup.getField(type);
    entryGroup.getField(symbol);
    entryGroup.getField(price);
    entryGroup.getField(size);
    partyGroup.getField(destination);
    entryGroup.getField(date);
    entryGroup.getField(daytime);

    symbol = m_originalNameSymbol[symbol];
    m_orderBooks[symbol][static_cast<OrderBook::Type>(static_cast<char>(type))]->update({ price, int(size), destination, s_convertToTimePoint(date.getValue(), daytime.getValue()) });
}

/**
 * @brief Receive the security list of all stock from BrokerageCenter.
 *
 * @param The list of names for the receiving stock list.
 *
 * @param SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID&) // override
{
    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find the NoRelatedSym in SecurityList !\n";
        return;
    }

    if (m_stockList.size() == 0) {
        std::lock_guard<std::mutex> slGuard(m_mutexStockList);

        FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
        FIX::Symbol symbol;
        m_stockList.clear();

        for (int i = 1; i <= numOfGroup; ++i) {
            message.getGroup(static_cast<unsigned int>(i), relatedSymGroup);
            relatedSymGroup.get(symbol);
            m_stockList.push_back(symbol);
        }

        createSymbolMap();
        initializePrices();
        initializeOrderBooks();

        m_cvStockList.notify_one();
    }

    // Resubscribe to order book data when reconnecting
    for (const auto& symbol : getSubscribedOrderBookList()) {
        subOrderBook(symbol);
    }

    // Resubscribe to candle stick data when reconnecting
    for (const auto& symbol : getSubscribedCandlestickList()) {
        subCandleData(symbol);
    }
}

/**
 * @brief Method to receive open/close/high/low for a specific ticker at a specific time (Data for candlestick chart).
 *
 * @param message as a SecurityStatus type object contains symbol, open, close, high, low, timestamp information.
 *
 * @param sessionID as the SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID&) // override
{
    FIX::Symbol symbol;
    FIX::StrikePrice open;
    FIX::HighPx high;
    FIX::LowPx low;
    FIX::LastPx close;
    FIX::Text timestamp;

    message.get(symbol);
    message.get(open);
    message.get(high);
    message.get(low);
    message.get(close);
    message.get(timestamp);

    symbol = m_originalNameSymbol[symbol];

    /**
     * @brief Logic for storing open price and check if ready. Open price stores the very first candle data open price for each ticker
     */
    if (!m_openPricesReady) {
        std::lock_guard<std::mutex> opGuard(m_mutexOpenPrices);
        if (m_openPrices.find(symbol.getString()) == m_openPrices.end()) {
            m_openPrices[symbol.getString()] = open;
            if (m_openPrices.size() == getStockList().size()) {
                m_openPricesReady = true;
            }
        }
    }

    try {
        getMainClient()->receiveCandlestickData(symbol, open, high, low, close, timestamp);
    } catch (...) {
        return;
    }
}

/*
 * @brief Send candle data request to BC
 */
void shift::FIXInitiator::sendCandleDataRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_RFQRequest));

    message.setField(FIX::RFQReqID(shift::crossguid::newGuid().str()));

    FIX50SP2::RFQRequest::NoRelatedSym relatedSymGroup;
    relatedSymGroup.setField(FIX::Symbol(symbol));
    message.addGroup(relatedSymGroup);

    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBTYPE_SUB);
    } else {
        message.setField(::FIXFIELD_SUBTYPE_UNSUB);
    }

    FIX::Session::sendToTarget(message);
}

/*
 * @brief send order book request to BC
 */
void shift::FIXInitiator::sendOrderBookRequest(const std::string& symbol, bool isSubscribed)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(shift::crossguid::newGuid().str()));
    if (isSubscribed) {
        message.setField(::FIXFIELD_SUBTYPE_SUB);
        message.setField(::FIXFIELD_UPDTYPE_INCRE);
    } else {
        message.setField(::FIXFIELD_SUBTYPE_UNSUB);
    }
    message.setField(::FIXFIELD_DOM_FULL); // Required by FIX

    FIX50SP2::MarketDataRequest::NoMDEntryTypes entryTypeGroup1;
    FIX50SP2::MarketDataRequest::NoMDEntryTypes entryTypeGroup2;
    entryTypeGroup1.setField(::FIXFIELD_ENTRY_BID);
    entryTypeGroup2.setField(::FIXFIELD_ENTRY_OFFER);
    message.addGroup(entryTypeGroup1);
    message.addGroup(entryTypeGroup2);

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    relatedSymGroup.setField(FIX::Symbol(symbol));
    message.addGroup(relatedSymGroup);

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
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getID()));
    message.setField(FIX::Symbol(m_symbolOriginalName[order.getSymbol()]));
    message.setField(FIX::Side(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::OrdType(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(order.getPrice()));

    FIX50SP2::NewOrderSingle::NoPartyIDs partyIDGroup;
    partyIDGroup.setField(::FIXFIELD_CLIENTID);
    if (username != "") {
        partyIDGroup.setField(FIX::PartyID(username));
    } else {
        partyIDGroup.setField(FIX::PartyID(m_username));
    }
    message.addGroup(partyIDGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method to get the open price of a certain symbol. Return the value from the open price book map.
 *
 * @param symbol The name of the symbol to be searched as a string.
 * @return The result open price as a double.
 */
double shift::FIXInitiator::getOpenPrice(const std::string& symbol)
{
    std::lock_guard<std::mutex> opGuard(m_mutexOpenPrices);
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
    std::lock_guard<std::mutex> slGuard(m_mutexStockList);
    return m_stockList;
}

void shift::FIXInitiator::fetchCompanyName(const std::string tickerName)
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

    std::lock_guard<std::mutex> cnGuard(m_mutexCompanyNames);
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
    std::lock_guard<std::mutex> cnGuard(m_mutexCompanyNames);
    return m_companyNames;
}

std::string shift::FIXInitiator::getCompanyName(const std::string& symbol)
{
    std::lock_guard<std::mutex> cnGuard(m_mutexCompanyNames);
    return m_companyNames[symbol];
}

void shift::FIXInitiator::subOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mutexSubscribedOrderBookSet);

    // It's ok to send repeated subscription requests.
    // A test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail.
    m_subscribedOrderBookSet.insert(symbol);
    sendOrderBookRequest(m_symbolOriginalName[symbol], true);
}

/**
 * @brief unsubscriber the orderbook
 */
void shift::FIXInitiator::unsubOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mutexSubscribedOrderBookSet);

    if (m_subscribedOrderBookSet.find(symbol) != m_subscribedOrderBookSet.end()) {
        m_subscribedOrderBookSet.erase(symbol);
        sendOrderBookRequest(m_symbolOriginalName[symbol], false);
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
    std::lock_guard<std::mutex> soblGuard(m_mutexSubscribedOrderBookSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedOrderBookSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}

void shift::FIXInitiator::subCandleData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mutexSubscribedCandleStickSet);

    // It's ok to send repeated subscription requests.
    // A test to see if it is already included in the set here is bad because
    // it would cause resubscription attempts during reconnections to fail.
    m_subscribedCandleStickSet.insert(symbol);
    sendCandleDataRequest(m_symbolOriginalName[symbol], true);
}

void shift::FIXInitiator::unsubCandleData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mutexSubscribedCandleStickSet);

    if (m_subscribedCandleStickSet.find(symbol) != m_subscribedCandleStickSet.end()) {
        m_subscribedCandleStickSet.erase(symbol);
        sendCandleDataRequest(m_symbolOriginalName[symbol], false);
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
    std::lock_guard<std::mutex> scslGuard(m_mutexSubscribedCandleStickSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedCandleStickSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}
