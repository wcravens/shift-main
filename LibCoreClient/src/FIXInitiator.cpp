#include "FIXInitiator.h"

#include "CoreClient.h"
#include "Exceptions.h"
#include "OrderBookGlobalAsk.h"
#include "OrderBookGlobalBid.h"
#include "OrderBookLocalAsk.h"
#include "OrderBookLocalBid.h"

#include <cmath>
#include <regex>
#include <thread>

#include <curl/curl.h>

#ifdef _WIN32
#include <terminal/Common.h>
#else
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

/**
 * @brief Default constructor for FIXInitiator object.
 */
shift::FIXInitiator::FIXInitiator()
    : m_connected{ false }
    , m_verbose{ false }
    , m_logonSuccess{ false }
    , m_openPricesReady{ false }
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

void shift::FIXInitiator::connectBrokerageCenter(const std::string& cfgFile, CoreClient* pmc, const std::string& password, bool verbose, int timeout)
{
    disconnectBrokerageCenter();

    m_username = pmc->getUsername();
    m_password = password;
    m_verbose = verbose;

    std::string senderCompID = m_username;
    convertToUpperCase(senderCompID);

    FIX::SessionSettings settings(cfgFile);
    FIX::Dictionary commonDict = settings.get();
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
    std::unique_lock<std::mutex> lsUniqueLock(m_mutex_logon);
    // Gets notify from fromAdmin() or onLogon()
    m_cv_logon.wait_for(lsUniqueLock, timeout * 1ms);
    auto t2 = std::chrono::steady_clock::now();
    timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    if (m_logonSuccess) {
        std::unique_lock<std::mutex> slUniqueLock(m_mutex_stockList);
        // Gets notify when security list is ready
        m_cv_stockList.wait_for(slUniqueLock, timeout * 1ms);

        attach(pmc);
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

    // Stock list is only updated when first connecting,
    // so this allows its update after a disconnection
    {
        std::lock_guard<std::mutex> slGuard(m_mutex_stockList);
        m_stockList.clear();
        m_originalName_symbol.clear();
        m_symbol_originalName.clear();
    }
    {
        std::lock_guard<std::mutex> cnGuard(m_mutex_companyNames);
        m_companyNames.clear();
    }

    // Subscriptions will reset automatically when disconnecting,
    // but we still need to clean up these sets
    {
        std::lock_guard<std::mutex> soblGuard(m_mutex_subscribedOrderBookSet);
        m_subscribedOrderBookSet.clear();
    }
    {
        std::lock_guard<std::mutex> scslGuard(m_mutex_subscribedCandleStickSet);
        m_subscribedCandleStickSet.clear();
    }

    return;
}

/**
 * @section WARNING
 * Don't call this function directly
 */
void shift::FIXInitiator::attach(shift::CoreClient* pCC, const std::string& password, int timeout)
{
    // Will attach both ways
    webClientSendUsername(pCC->getUsername());
    // TODO: add auth in BC and add lock after successful logon; use password to auth

    m_username_client[pCC->getUsername()] = pCC;
    pCC->attach(*this);
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
    for (auto it = m_username_client.begin(); it != m_username_client.end(); ++it) {
        if (it->first != m_username) {
            clientsVector.push_back(it->second);
        }
    }
    return clientsVector;
}

shift::CoreClient* shift::FIXInitiator::getMainClient()
{
    return getClientByName(m_username);
}

/**
 * @brief get CoreClient by client name.
 * 
 * @param name as a string to provide the client name.
 */
shift::CoreClient* shift::FIXInitiator::getClientByName(const std::string& name)
{
    if (m_username_client.find(name) != m_username_client.end()) {
        return m_username_client[name];
    } else {
        // debugDump("Name not found: " + name);
        throw std::range_error("Name not found: " + name);
    }
    return nullptr;
}

bool shift::FIXInitiator::isConnected()
{
    return m_connected;
}

inline void shift::FIXInitiator::convertToUpperCase(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
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
 * @brief Method to initialize prices for every symbol 
 * from the stock list
 */
inline void shift::FIXInitiator::initializePrices()
{
    for (const auto& symbol : m_stockList) {
        m_lastPrices[symbol] = 0.0;
    }

    std::lock_guard<std::mutex> opGuard(m_mutex_openPrices);
    m_openPrices.clear();
}

/**
 * @brief Method to initialize order books for every symbol 
 * from the stock list
 */
inline void shift::FIXInitiator::initializeOrderBooks()
{
    for (const auto& symbol : m_stockList) {
        if (m_orderBooks[symbol]['A']) {
            delete m_orderBooks[symbol]['A'];
        }
        m_orderBooks[symbol]['A'] = new OrderBookGlobalAsk();

        if (m_orderBooks[symbol]['B']) {
            delete m_orderBooks[symbol]['B'];
        }
        m_orderBooks[symbol]['B'] = new OrderBookGlobalBid();

        if (m_orderBooks[symbol]['a']) {
            delete m_orderBooks[symbol]['a'];
        }
        m_orderBooks[symbol]['a'] = new OrderBookLocalAsk();

        if (m_orderBooks[symbol]['b']) {
            delete m_orderBooks[symbol]['b'];
        }
        m_orderBooks[symbol]['b'] = new OrderBookLocalBid();
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
    m_cv_logon.notify_one();
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
void shift::FIXInitiator::toAdmin(FIX::Message& message, const FIX::SessionID& session_id) // override
{
    if (FIX::MsgType_Logon == message.getHeader().getField(FIX::FIELD::MsgType)) {
        // set username and password in logon message
        FIXT11::Logon::NoMsgTypes msgTypeGroup;
        msgTypeGroup.set(FIX::RefMsgType(m_username));
        message.addGroup(msgTypeGroup);
        msgTypeGroup.set(FIX::RefMsgType(m_password));
        message.addGroup(msgTypeGroup);
    }
}

/**
 * @brief Method to communicate with FIX server.
 */
void shift::FIXInitiator::toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) // override
{
    try {
        FIX::PossDupFlag possDupFlag;
        message.getHeader().getField(possDupFlag);
        if (possDupFlag)
            throw FIX::DoNotSend();
    } catch (FIX::FieldNotFound&) {
    }
}

/**
 * @brief Method for passing message to core client
 */
void shift::FIXInitiator::fromAdmin(const FIX::Message& message, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) // override
// void shift::FIXInitiator::fromAdmin(const FIX::Message& message)
{
    if (FIX::MsgType_Logout == message.getHeader().getField(FIX::FIELD::MsgType)) {
        FIX::Text text = message.getField(FIX::FIELD::Text);
        if (text == "Rejected Logon Attempt") {
            m_logonSuccess = false;
            m_cv_logon.notify_one();
        }
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
    FIX::ExecType execType;
    message.get(execType);

    if (execType == ::FIXFIELD_EXECTYPE_TRADE && m_connected) {
        FIX::NoPartyIDs numOfGroup;
        message.get(numOfGroup);
        if (numOfGroup < 2) {
            cout << "Can't find the Parties in ExecutionReport!" << endl;
            return;
        }

        FIX::OrderID orderID;
        FIX::Symbol symbol;
        FIX::OrdType orderType;
        FIX::Price price;
        FIX::CumQty size;
        FIX::Text confirmTime;

        FIX50SP2::ExecutionReport::NoPartyIDs partyGroup;
        FIX::PartyID clientID;
        FIX::PartyID destination;

        message.get(orderID);
        message.get(symbol);
        message.get(orderType);
        message.get(price);
        message.get(size);
        message.get(confirmTime);

        message.getGroup(1, partyGroup);
        partyGroup.get(clientID);
        message.getGroup(2, partyGroup);
        partyGroup.get(destination);

        symbol = m_originalName_symbol[symbol];

        /***** ASK ABOUT DARK POOL PRICE *****/
        // ignore dark pool price
        double price100 = price * 100;
        int priceInt = std::round(price100);
        if (std::abs(price100 - priceInt) < 0.000001) {
            m_lastPrices[symbol] = price;
            try {
                getMainClient()->receiveLastPrice(symbol);
            } catch (...) {
                return;
            }
        }
        return;
    }

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
 * @brief Method to receive Global/Local orders OR portfolio update from Brokerage Center.
 * 
 * @param message as a QuoteAcknowledgement type object contains the current accepting order information.
 * 
 * @param sessionID as the SessionID for the current communication. 
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID& sessionID) // override
{
    FIX::Account username;
    FIX::Text headline;

    message.get(username);
    message.get(headline);

    if (headline == "portfolio") { // Case for receiving a portfolio update
        while (!m_connected)
            ;

        FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;

        message.getGroup(1, quoteSetGroup);

        FIX::QuoteSetID clientID;
        FIX::UnderlyingSymbol symbol;
        FIX::UnderlyingSecurityID totalSharesRaw;
        FIX::UnderlyingStrikePrice price;
        FIX::UnderlyingContractMultiplier totalRealizedPL;
        FIX::UnderlyingCouponRate totalBuyingPower;
        FIX::UnderlyingSymbolSfx sharesRaw;
        FIX::UnderlyingIssuer realizedPLRaw;

        quoteSetGroup.get(clientID);
        quoteSetGroup.get(symbol);
        quoteSetGroup.get(totalSharesRaw);
        quoteSetGroup.get(price);
        quoteSetGroup.get(totalRealizedPL);
        quoteSetGroup.get(totalBuyingPower);
        quoteSetGroup.get(sharesRaw);
        quoteSetGroup.get(realizedPLRaw);

        symbol = m_originalName_symbol[symbol];

        std::stringstream ss;
        int totalShares, shares;
        double realizedPL;

        ss.str(totalSharesRaw);
        ss >> totalShares;
        ss.clear();
        ss.str(sharesRaw);
        ss >> shares;
        ss.clear();
        ss.str(realizedPLRaw);
        ss >> realizedPL;

        try {
            getClientByName(username)->storePortfolio(totalRealizedPL, totalBuyingPower, totalShares, symbol, shares, price, realizedPL);
            getClientByName(username)->receivePortfolio(symbol);
        } catch (...) {
            return;
        }
    } else if (headline == "orderbook") {
        FIX::NoQuoteSets n;
        message.get(n);
        if (!n)
            return;

        FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;
        /***************** The following QuoteSetID is not in any use ******************/
        FIX::QuoteSetID quoteSetID;

        // Accepting type with their underlying type as followed comments
        FIX::UnderlyingSymbol symbol; //string
        FIX::UnderlyingSecurityID destination; //string
        FIX::UnderlyingStrikePrice price; //double
        FIX::UnderlyingOptAttribute type; //char
        FIX::UnderlyingContractMultiplier size; //double
        FIX::UnderlyingCouponRate time; //double

        std::vector<shift::OrderBookEntry> orderBook;

        for (int i = 1; i <= n; i++) {
            message.getGroup(i, quoteSetGroup);

            quoteSetGroup.get(quoteSetID);
            quoteSetGroup.get(symbol);
            quoteSetGroup.get(destination);
            quoteSetGroup.get(price);
            quoteSetGroup.get(type);
            quoteSetGroup.get(size);
            quoteSetGroup.get(time);

            symbol = m_originalName_symbol[symbol];
            shift::OrderBookEntry entry(type, symbol, price, size, destination, time);
            orderBook.push_back(entry);
        }

        try {
            m_orderBooks[symbol][type]->setOrderBook(orderBook);
        } catch (std::exception e) {
            debugDump(symbol.getString() + " doesn't work");
        }
    } else if (headline == "quoteHistory") {
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
        // FIX::ExecType orderType;

        std::vector<shift::Order> waitingList;

        for (int i = 1; i <= n; i++) {
            message.getGroup(i, quoteSetGroup);

            quoteSetGroup.get(userID);
            quoteSetGroup.get(symbol);
            quoteSetGroup.get(orderSize);
            quoteSetGroup.get(orderID);
            quoteSetGroup.get(price);
            quoteSetGroup.get(orderType);

            symbol = m_originalName_symbol[symbol];
            int size = atoi((std::string(orderSize)).c_str());
            shift::Order::Type type = shift::Order::Type(
                char(atoi(((std::string)orderType).c_str())));

            if (size != 0) {
                shift::Order o(symbol, price, size, type, orderID);
                waitingList.push_back(o);
            }
        }

        try {
            // store the data in target client
            getClientByName(username)->storeWaitingList(waitingList);
            // notify target client
            getClientByName(username)->receiveWaitingList();
        } catch (...) {
            return;
        }
    }
}

/**
 * @brief   Method to update already saved order book 
 *          when receiving order book updates from Brokerage Center
 * @param   Message as an MarketDataIncrementalRefresh type object 
 *          contains updated order book information.
 * @param   sessionID as the SessionID for the current communication.  
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID& sessionID) // override
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

    FIX::MDEntryType bookType;
    FIX::Symbol symbol;
    FIX::MDEntryPx price;
    FIX::MDEntrySize size;
    FIX::Text time;
    FIX::PartyID destination;

    entryGroup.get(bookType);
    entryGroup.get(symbol);
    entryGroup.get(price);
    entryGroup.get(size);
    entryGroup.get(time);
    partyGroup.get(destination);

    symbol = m_originalName_symbol[symbol];
    shift::OrderBookEntry entry(bookType, symbol, price, size, destination, std::stod(time));
    m_orderBooks[symbol][bookType]->update(entry);
}

/**
 * @brief Receive the security list of all stock from BrokerageCenter.
 * 
 * @param The list of names for the receiving stock list.
 * 
 * @param SessionID for the current communication.
 */
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find the NoRelatedSym in SecurityList !\n";
        return;
    }

    if (m_stockList.size() == 0) {
        std::lock_guard<std::mutex> slGuard(m_mutex_stockList);

        FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
        FIX::Symbol symbol;
        m_stockList.clear();

        for (int i = 1; i <= numOfGroup; ++i) {
            message.getGroup(i, relatedSymGroup);
            relatedSymGroup.get(symbol);
            m_stockList.push_back(symbol);
        }

        createSymbolMap();
        initializePrices();
        initializeOrderBooks();

        m_cv_stockList.notify_one();
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
void shift::FIXInitiator::onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID& sessionID) // override
{
    FIX::Symbol symbol;
    FIX::SecurityID timestamp;
    FIX::StrikePrice open;
    FIX::HighPx high;
    FIX::LowPx low;
    FIX::LastPx close;

    message.get(symbol);
    message.get(timestamp);
    message.get(open);
    message.get(high);
    message.get(low);
    message.get(close);

    symbol = m_originalName_symbol[symbol];

    /**
     * @brief Logic for storing open price and check if ready. Open price stores the very first candle data open price for each ticker
     */
    if (!m_openPricesReady) {
        std::lock_guard<std::mutex> opGuard(m_mutex_openPrices);
        if (m_openPrices.find(symbol.getString()) == m_openPrices.end()) {
            m_openPrices[symbol.getString()] = open;
            if (m_openPrices.size() == getStockList().size()) {
                m_openPricesReady = true;
            }
        }
    }

    try {
        // getMainClient()->receiveCandleData(symbol, open, high, low, close, timestamp);
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
    relatedSymGroup.set(FIX::Symbol(symbol));
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
    entryTypeGroup1.set(::FIXFIELD_ENTRY_BID);
    entryTypeGroup2.set(::FIXFIELD_ENTRY_OFFER);
    message.addGroup(entryTypeGroup1);
    message.addGroup(entryTypeGroup2);

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    relatedSymGroup.set(FIX::Symbol(symbol));
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
    message.setField(FIX::Symbol(m_symbol_originalName[order.getSymbol()]));
    message.setField(FIX::Side(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::OrdType(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(order.getPrice()));

    FIX50SP2::NewOrderSingle::NoPartyIDs partyIDGroup;
    partyIDGroup.set(::FIXFIELD_CLIENTID);
    if (username != "") {
        partyIDGroup.set(FIX::PartyID(username));
    } else {
        partyIDGroup.set(FIX::PartyID(m_username));
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
double shift::FIXInitiator::getOpenPriceBySymbol(const std::string& symbol)
{
    std::lock_guard<std::mutex> opGuard(m_mutex_openPrices);
    if (m_openPrices.find(symbol) != m_openPrices.end()) {
        return m_openPrices[symbol];
    } else {
        return 0.0;
    }
}

/**
 * @brief Method to get the last price of a certain symbol. Return the value from the last price book map.
 * 
 * @param symbol The name of the symbol to be searched as a string.
 * @return The result last price as a double.
 */
double shift::FIXInitiator::getLastPriceBySymbol(const std::string& symbol)
{
    return m_lastPrices[symbol];
}

/**
 * @brief Method to get the BestPrice object for the target symbol.
 * 
 * @param symbol The name of the symbol to be searched.
 * 
 * @return shift::BestPrice for the target symbol.
 */
shift::BestPrice shift::FIXInitiator::getBestPriceBySymbol(const std::string& symbol)
{
    shift::BestPrice bp = shift::BestPrice(m_orderBooks[symbol]['B']->getBestPrice(),
        m_orderBooks[symbol]['B']->getBestSize(),
        m_orderBooks[symbol]['A']->getBestPrice(),
        m_orderBooks[symbol]['A']->getBestSize(),
        m_orderBooks[symbol]['b']->getBestPrice(),
        m_orderBooks[symbol]['b']->getBestSize(),
        m_orderBooks[symbol]['a']->getBestPrice(),
        m_orderBooks[symbol]['a']->getBestSize());

    return bp;
}

/**
 * @brief Method to get the corresponding order book by symbol name and entry type.
 * 
 * @param symbol The target symbol to find from the order book map.
 * 
 * @param type The target entry type (Global bid "B"/ask "A", local bid "b"/ask "a")
 */
std::vector<shift::OrderBookEntry> shift::FIXInitiator::getOrderBook(const std::string& symbol, char type)
{
    if (m_orderBooks.find(symbol) == m_orderBooks.end()) {
        throw "There is no Order Book for symbol " + symbol;
    }

    if (m_orderBooks[symbol].find(type) == m_orderBooks[symbol].end()) {
        throw "Order Book type is invalid";
    }

    return m_orderBooks[symbol][type]->getOrderBook();
}

/**
 * @brief Method to get the corresponding order book with destination by symbol name and entry type.
 * 
 * @param symbol The target symbol to find from the order book map.
 * 
 * @param type The target entry type (Global bid "B"/ask "A", local bid "b"/ask "a")
 */
std::vector<shift::OrderBookEntry> shift::FIXInitiator::getOrderBookWithDestination(const std::string& symbol, char type)
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
    std::lock_guard<std::mutex> slGuard(m_mutex_stockList);
    return m_stockList;
}

/* static */ int shift::FIXInitiator::s_writer(char* data, size_t size, size_t nmemb, std::string* buffer)
{
    int result = 0;

    if (buffer != NULL) {
        buffer->append(data, size * nmemb);
        result = size * nmemb;
    }

    return result;
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
        // Since curl is a C library, need to cast url into c string to process, same for HTML
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, shift::FIXInitiator::s_writer); // manages the required buffer size
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

    std::lock_guard<std::mutex> cnGuard(m_mutex_companyNames);
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
    std::lock_guard<std::mutex> cnGuard(m_mutex_companyNames);
    return m_companyNames;
}

std::string shift::FIXInitiator::getCompanyNameBySymbol(const std::string& symbol)
{
    std::lock_guard<std::mutex> cnGuard(m_mutex_companyNames);
    return m_companyNames[symbol];
}

bool shift::FIXInitiator::subOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mutex_subscribedOrderBookSet);

    if (m_subscribedOrderBookSet.find(symbol) == m_subscribedOrderBookSet.end()) {
        m_subscribedOrderBookSet.insert(symbol);
        sendOrderBookRequest(m_symbol_originalName[symbol], true);
    }

    return true;
}

/**
 * @brief unsubscriber the orderbook
 */
bool shift::FIXInitiator::unsubOrderBook(const std::string& symbol)
{
    std::lock_guard<std::mutex> soblGuard(m_mutex_subscribedOrderBookSet);

    if (m_subscribedOrderBookSet.find(symbol) != m_subscribedOrderBookSet.end()) {
        m_subscribedOrderBookSet.erase(symbol);
        sendOrderBookRequest(m_symbol_originalName[symbol], false);
    }

    return true;
}

bool shift::FIXInitiator::subAllOrderBook()
{
    bool success = true;

    for (const auto& symbol : getStockList()) {
        if (!subOrderBook(symbol)) {
            success = false;
        }
    }

    return success;
}

bool shift::FIXInitiator::unsubAllOrderBook()
{
    bool success = true;

    for (const auto& symbol : getStockList()) {
        if (!unsubOrderBook(symbol)) {
            success = false;
        }
    }

    return success;
}

/**
 * @brief Method to get the list of symbols who currently 
 * subscribed their order book.
 * 
 * @return  A vector of symbols who subscribed their order book.
 */
std::vector<std::string> shift::FIXInitiator::getSubscribedOrderBookList()
{
    std::lock_guard<std::mutex> soblGuard(m_mutex_subscribedOrderBookSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedOrderBookSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}

bool shift::FIXInitiator::subCandleData(const std::string& symbol)
{
    if (getLastPriceBySymbol(symbol) == 0.0) {
        return false;
    }

    std::lock_guard<std::mutex> scslGuard(m_mutex_subscribedCandleStickSet);

    if (m_subscribedCandleStickSet.find(symbol) == m_subscribedCandleStickSet.end()) {
        m_subscribedCandleStickSet.insert(symbol);
        sendCandleDataRequest(m_symbol_originalName[symbol], true);
    }

    return true;
}

bool shift::FIXInitiator::unsubCandleData(const std::string& symbol)
{
    std::lock_guard<std::mutex> scslGuard(m_mutex_subscribedCandleStickSet);

    if (m_subscribedCandleStickSet.find(symbol) != m_subscribedCandleStickSet.end()) {
        m_subscribedCandleStickSet.erase(symbol);
        sendCandleDataRequest(m_symbol_originalName[symbol], false);
    }

    return true;
}

bool shift::FIXInitiator::subAllCandleData()
{
    bool success = true;

    for (const auto& symbol : getStockList()) {
        if (!subCandleData(symbol)) {
            success = false;
        }
    }

    return success;
}

bool shift::FIXInitiator::unsubAllCandleData()
{
    bool success = true;

    for (const auto& symbol : getStockList()) {
        if (!unsubCandleData(symbol)) {
            success = false;
        }
    }

    return success;
}

/**
 * @brief Method to get the list of symbols who currently 
 * subscribed their candle data.
 * 
 * @return A vector of symbols who subscribed their candle data.
 */
std::vector<std::string> shift::FIXInitiator::getSubscribedCandlestickList()
{
    std::lock_guard<std::mutex> scslGuard(m_mutex_subscribedCandleStickSet);

    std::vector<std::string> subscriptionList;

    for (std::string symbol : m_subscribedCandleStickSet) {
        subscriptionList.push_back(symbol);
    }

    return subscriptionList;
}
