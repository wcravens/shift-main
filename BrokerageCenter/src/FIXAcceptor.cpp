/* 
** This file contains the implementation of functions in FIXAcceptor.h
**/

#include "FIXAcceptor.h"

#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

#define SHOW_INPUT_MSG false

/* static */ std::string FIXAcceptor::s_senderID;

// Predefined constant FIX message (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1");
static const auto& FIXFIELD_MSGTY_MASSQUOTEACK = FIX::MsgType(FIX::MsgType_MassQuoteAcknowledgement);
static const auto& FIXFIELD_QUOTESTAT = FIX::QuoteStatus(0); // 0 = Accepted
static const auto& FIXFIELD_TXT_QH = FIX::Text("quoteHistory");
static const auto& FIXFIELD_MDUPDATE_CHANGE = FIX::MDUpdateAction('1');
static const auto& FIXFIELD_EXECBROKER = FIX::PartyRole(1); // 1 = ExecBroker in FIX4.2
static const auto& FIXFIELD_EXECTYPE_NEW = FIX::ExecType('0'); // 0 = New
static const auto& FIXFIELD_SIDE_BUY = FIX::Side('1'); // 1 = Buy
static const auto& FIXFIELD_LEAVQTY_100 = FIX::LeavesQty(100); // Quantity open for further execution
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = Client ID in FIX4.2
static const auto& FIXFIELD_QUOTESETID = FIX::QuoteSetID("");
static const auto& FIXFIELD_TXT_OB = FIX::Text("orderbook");
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType('F'); // F = Trade
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0); // Quantity open for further execution
static const auto& FIXFIELD_MSGTY_POSIREPORT = FIX::MsgType(FIX::MsgType_PositionReport);

FIXAcceptor::~FIXAcceptor() // override
{
    disconnectClients();
}

/*static*/ FIXAcceptor* FIXAcceptor::instance()
{
    static FIXAcceptor s_FIXAccInst;
    return &s_FIXAccInst;
}

void FIXAcceptor::connectClients(const std::string& configFile, bool verbose)
{
    disconnectClients();

    FIX::SessionSettings settings(configFile);
    m_clientNames = DBConnector::s_readRowsOfField("SELECT username FROM traders;");
    std::transform(m_clientNames.begin(), m_clientNames.end(), m_clientNames.begin(), ::toUpper);

    const FIX::Dictionary commonDict = settings.get();
    for (const auto& name : m_clientNames) {
        FIX::SessionID sid(commonDict.getString("BeginString"), commonDict.getString("SenderCompID") // e.g. BROKERAGECENTER
            ,
            name // TargetCompID
        );

        FIX::Dictionary dict;
        settings.set(sid, std::move(dict));
    }

    if (commonDict.has("FileLogPath")) {
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
    } else {
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, verbose));
    }

    if (commonDict.has("FileStorePath")) {
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
    } else {
        m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    }

    m_socketAcceptorPtr.reset(new FIX::SocketAcceptor(*instance(), *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Acceptor is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_socketAcceptorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXAcceptor::disconnectClients()
{
    if (!m_socketAcceptorPtr)
        return;

    cout << '\n'
         << COLOR "Acceptor is stopping..." NO_COLOR << '\n'
         << flush;

    m_socketAcceptorPtr->stop();
    m_socketAcceptorPtr = nullptr;
    m_messageStoreFactoryPtr = nullptr;
    m_logFactoryPtr = nullptr;
}

/**
 * @brief Send Portfolio Item to LC
 */
void FIXAcceptor::sendPortfolioItem(const std::string& userName, const std::string& clientID, const PortfolioItem& item)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIXFIELD_MSGTY_POSIREPORT);

    message.setField(FIX::PosMaintRptID(shift::crossguid::newGuid().str()));
    message.setField(FIX::ClearingBusinessDate("20181102")); // Required by FIX
    message.setField(FIX::Symbol(item.getSymbol()));
    message.setField(FIX::SecurityType("CS"));
    message.setField(FIX::SettlPrice(item.getSummaryPrice()));
    message.setField(FIX::PriceDelta(item.getPL()));

    FIX50SP2::PositionReport::NoPartyIDs userNameGroup;
    userNameGroup.setField(::FIXFIELD_CLIENTID);
    userNameGroup.setField(FIX::PartyID(userName));
    message.addGroup(userNameGroup);

    FIX50SP2::PositionReport::NoPositions qtyGroup;
    qtyGroup.setField(FIX::LongQty(item.getLongShares()));
    qtyGroup.setField(FIX::ShortQty(item.getShortShares()));
    message.addGroup(qtyGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send Portfolio Summary to LC
 */
void FIXAcceptor::sendPortfolioSummary(const std::string& userName, const std::string& clientID, const PortfolioSummary& summary)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIXFIELD_MSGTY_POSIREPORT);

    message.setField(FIX::PosMaintRptID(shift::crossguid::newGuid().str()));
    message.setField(FIX::ClearingBusinessDate("20181102")); // Required by FIX
    message.setField(FIX::Symbol("CASH"));
    message.setField(FIX::SecurityType("CASH"));
    message.setField(FIX::PriceDelta(summary.getTotalPL()));

    FIX50SP2::PositionReport::NoPartyIDs userNameGroup;
    userNameGroup.setField(::FIXFIELD_CLIENTID);
    userNameGroup.setField(FIX::PartyID(userName));
    message.addGroup(userNameGroup);

    FIX50SP2::PositionReport::NoPositions qtyGroup;
    qtyGroup.setField(FIX::LongQty(summary.getTotalShares()));
    message.addGroup(qtyGroup);

    FIX50SP2::PositionReport::NoPosAmt buyingPowerGroup;
    buyingPowerGroup.setField(FIX::PosAmt(summary.getBuyingPower()));
    message.addGroup(buyingPowerGroup);

    FIX50SP2::PositionReport::NoPosAmt holdingBalanceGroup;
    holdingBalanceGroup.setField(FIX::PosAmt(summary.getHoldingBalance()));
    message.addGroup(holdingBalanceGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief 
 */
void FIXAcceptor::sendQuoteHistory(const std::string& userName, const std::unordered_map<std::string, Quote>& quotes)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX50SP2::MassQuoteAcknowledgement message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(::FIXFIELD_MSGTY_MASSQUOTEACK);

    message.setField(::FIXFIELD_QUOTESTAT); // Required by FIX
    message.setField(::FIXFIELD_TXT_QH);
    message.setField(FIX::Account(userName));

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;
    for (const auto& i : quotes) {
        auto& quote = i.second;
        quoteSetGroup.setField(FIX::QuoteSetID(clientID));
        quoteSetGroup.setField(FIX::UnderlyingSecurityID(quote.getOrderID()));
        quoteSetGroup.setField(FIX::UnderlyingSymbol(quote.getSymbol()));
        quoteSetGroup.setField(FIX::UnderlyingStrikePrice(quote.getPrice()));
        quoteSetGroup.setField(FIX::UnderlyingSymbolSfx(std::to_string(quote.getShareSize()))); // FIXME: replaceb by other field
        quoteSetGroup.setField(FIX::UnderlyingIssuer(std::to_string(quote.getOrderType())));
        message.addGroup(quoteSetGroup);
    }
    FIX::Session::sendToTarget(message);
}

/**
 * @brief   Send order book update to one client
 * @param   userName as a string to provide the name of client
 * @param   update as an OrderBook object to provide price and others
 */
void FIXAcceptor::SendOrderbookUpdate(const std::string& userName, const OrderBookEntry& update)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATE_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(char(update.getType())));
    entryGroup.setField(FIX::Symbol(update.getSymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(update.getSize()));
    entryGroup.setField(FIX::Text(std::to_string(update.getTime())));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(::FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(update.getDestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief   Send order book update to all clients
 * @param   clientList as a list to provide the name of clients
 * @param   update as an OrderBook object to provide price and others
 */
void FIXAcceptor::SendOrderbookUpdate2All(const std::unordered_set<std::string>& clientList, const OrderBookEntry& update)
{
    for (const auto& userName : clientList) {
        SendOrderbookUpdate(userName, update);
    }
}

/**
 * @brief Send order book update to all clients
 */
void FIXAcceptor::sendNewBook2all(const std::unordered_set<std::string>& clientList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    for (const auto& userName : clientList) {
        sendOrderBook(userName, orderBookName);
    }
}

/**
 * @brief Sending the order confirmation to the client, 
 * because report.status usual set to 1, 
 * Then the client will notify it is a confirmation
 */
void FIXAcceptor::sendConfirmationReport(const Report& report)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(report.clientID); // clientID in report is userName
    if (CSTR_NULL == clientID) {
        cout << report.clientID << " does not exist !" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID));
    message.setField(FIX::ClOrdID(report.clientID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_NEW); // Required by FIX
    message.setField(FIX::OrdStatus(report.status));
    message.setField(FIX::Symbol(report.symbol));
    message.setField(::FIXFIELD_SIDE_BUY); // Required by FIX
    message.setField(FIX::OrdType(report.orderType));
    message.setField(::FIXFIELD_LEAVQTY_100); // Required by FIX
    message.setField(FIX::CumQty(report.shareSize));
    message.setField(FIX::AvgPx(report.price)); //report.getAvgPx()
    message.setField(FIX::TransactTime(report.serverTime, 6));
    message.setField(FIX::EffectiveTime(report.execTime, 6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.setField(::FIXFIELD_CLIENTID);
    idGroup.setField(FIX::PartyID(report.clientID));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

static void s_setAddGroupIntoQuoteAckMsg(FIX::Message& message, FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets& quoteSetGroup, const OrderBookEntry& odrBk)
{
    quoteSetGroup.setField(::FIXFIELD_QUOTESETID);
    quoteSetGroup.setField(FIX::UnderlyingSymbol(odrBk.getSymbol()));
    quoteSetGroup.setField(FIX::UnderlyingSecurityID(odrBk.getDestination()));
    quoteSetGroup.setField(FIX::UnderlyingStrikePrice(odrBk.getPrice()));
    quoteSetGroup.setField(FIX::UnderlyingOptAttribute(char(odrBk.getType())));
    quoteSetGroup.setField(FIX::UnderlyingContractMultiplier(odrBk.getSize()));
    quoteSetGroup.setField(FIX::UnderlyingCouponRate(odrBk.getTime()));
    message.addGroup(quoteSetGroup);
}

/**
 * @brief Send complete order book by type
 */
void FIXAcceptor::sendOrderBook(const std::string& userName, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(::FIXFIELD_MSGTY_MASSQUOTEACK);

    message.setField(::FIXFIELD_QUOTESTAT); // Required by FIX
    message.setField(FIX::Account(userName));
    message.setField(::FIXFIELD_TXT_OB);

    using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;
    const auto obt = orderBookName.begin()->second.begin()->second.getType();

    if (obt == OBT::GLB_BID || obt == OBT::LOC_BID) { //reverse the Bid/bid orderbook order
        for (auto ri = orderBookName.crbegin(); ri != orderBookName.crend(); ++ri) {
            for (const auto& j : ri->second)
                ::s_setAddGroupIntoQuoteAckMsg(message, quoteSetGroup, j.second);
        }
    } else { // *_ASK
        for (const auto& i : orderBookName) {
            for (const auto& j : i.second)
                ::s_setAddGroupIntoQuoteAckMsg(message, quoteSetGroup, j.second);
        }
    }

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::sendTempStockSummary(const std::string& userName, const TempStockSummary& tempSS)
{ //for candlestick data
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityStatus));

    message.setField(FIX::Symbol(tempSS.getSymbol()));
    message.setField(FIX::Text(std::to_string(tempSS.getTempTimeFrom()))); // no need for timestamp
    message.setField(FIX::StrikePrice(tempSS.getTempOpenPrice())); // Open price
    message.setField(FIX::HighPx(tempSS.getTempHighPrice())); // High price
    message.setField(FIX::LowPx(tempSS.getTempLowPrice())); // Low price
    message.setField(FIX::LastPx(tempSS.getTempClosePrice())); // Close price

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send the latest stock price to LC 
 * 
 * @param userName as a string that converted to ClientID
 * 
 * @param transac as a Transaction object to provide latest price
 */
void FIXAcceptor::sendLatestStockPrice(const std::string& userName, const Transaction& transac)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(transac.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_TRADE);
    message.setField(FIX::OrdStatus(transac.orderDecision));
    message.setField(FIX::Symbol(transac.symbol));
    message.setField(FIX::Side(transac.orderType));
    message.setField(FIX::OrdType(transac.orderType));
    message.setField(FIX::Price(transac.price));
    message.setField(::FIXFIELD_LEAVQTY_0);
    message.setField(FIX::CumQty(transac.execQty));
    message.setField(FIX::EffectiveTime(transac.execTime, 6));
    message.setField(FIX::TransactTime(transac.serverTime, 6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.setField(::FIXFIELD_CLIENTID);
    idGroup.setField(FIX::PartyID(transac.traderID));
    message.addGroup(idGroup);

    FIX50SP2::ExecutionReport::NoPartyIDs brokerGroup;
    brokerGroup.setField(::FIXFIELD_EXECBROKER);
    brokerGroup.setField(FIX::PartyID(transac.destination));
    message.addGroup(brokerGroup);

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::sendLatestStockPrice2All(const Transaction& transac)
{
    for (const auto& i : BCDocuments::instance()->getClientList()) {
        sendLatestStockPrice(i.first, transac);
    }
}

inline bool FIXAcceptor::subscribeOrderBook(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::instance()->manageStockOrderBookClient(true, symbol, userName);
}

inline bool FIXAcceptor::unsubscribeOrderBook(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::instance()->manageStockOrderBookClient(false, symbol, userName);
}

inline bool FIXAcceptor::subscribeCandleData(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::instance()->manageCandleStickDataClient(true, userName, symbol);
}

inline bool FIXAcceptor::unsubscribeCandleData(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::instance()->manageCandleStickDataClient(false, userName, symbol);
}

/**
 * @brief Method called when a new Session was created. 
 * Set Sender and Target Comp ID.
 */
void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID();
    /* SenderID: BROKERAGECENTER */
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    auto& clntID = sessionID.getTargetCompID();
    const auto& userName = BCDocuments::instance()->getClientNameByID(clntID);
    cout << "Logon: " << clntID << endl;
    if (CSTR_NULL == clntID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    // send security list of all stocks to client when client login
    sendSecurityList(userName, BCDocuments::instance()->getSymbols());

    BCDocuments::instance()->sendHistoryToClient(userName);
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    auto& clntID = sessionID.getTargetCompID();
    cout << "Logout targetID: " << clntID << endl;
    if (::toUpper(clntID) == "WEBCLIENTID")
        return;

    const auto& userName = BCDocuments::instance()->getClientNameByID(clntID);
    if (CSTR_NULL == clntID) {
        cout << userName << " does not exist !" << endl;
        return;
    }
    cout << "Logon username: " << userName << endl;
    BCDocuments::instance()->unregisterClientInDocs(userName); //remove the client in connection
    BCDocuments::instance()->removeClientFromStocks(userName);
    BCDocuments::instance()->removeClientFromCandles(userName);
    cout << endl
         << "Logout - " << sessionID << endl;
}

void FIXAcceptor::toAdmin(FIX::Message& message, const FIX::SessionID&) // override
{
    (void)message;
#if SHOW_INPUT_MSG
    cout << "BC toAdmin: " << message.toString() << endl;
#endif
}

void FIXAcceptor::toApp(FIX::Message& message, const FIX::SessionID&) throw(FIX::DoNotSend) // override
{
    try {
        FIX::PossDupFlag possDupFlag;
        message.getHeader().getField(possDupFlag);
        if (possDupFlag)
            throw FIX::DoNotSend();
    } catch (FIX::FieldNotFound&) {
    }
}

void FIXAcceptor::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) // override
{
    if (FIX::MsgType_Logon != message.getHeader().getField(FIX::FIELD::MsgType))
        return;

    std::string userName, password;
    const auto& clntID = sessionID.getTargetCompID();

    FIXT11::Logon::NoMsgTypes msgTypeGroup;
    message.getGroup(1, msgTypeGroup);
    userName = msgTypeGroup.getField(FIX::FIELD::RefMsgType);
    message.getGroup(2, msgTypeGroup);
    password = msgTypeGroup.getField(FIX::FIELD::RefMsgType);

    auto pswCol = DBConnector::s_readRowsOfField("SELECT password FROM traders WHERE username = '" + userName + "';");
    if (pswCol.size() && pswCol.front() == password) {
        BCDocuments::instance()->registerClient(clntID, userName);
        cout << COLOR_PROMPT "Authentication successful for user: " << userName << NO_COLOR << endl;
    } else {
        cout << COLOR_WARNING "User name or password was wrong." NO_COLOR << endl;
        throw FIX::RejectLogon();
    }
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
#if SHOW_INPUT_MSG
    cout << endl
         << "IN: " << message << endl;
#endif
}

/*
 * @brief Receive OrderBook request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);

    if (numOfGroup < 1) {
        cout << "Cannot find Symbol in MarketDataRequest!" << endl;
        return;
    }

    FIX::SubscriptionRequestType isSubscribed;
    message.get(isSubscribed);

    bool (FIXAcceptor::*pmf)(const std::string&, const std::string&) = nullptr;
    if (isSubscribed == '1') {
        pmf = &FIXAcceptor::subscribeOrderBook;
    } else {
        pmf = &FIXAcceptor::unsubscribeOrderBook;
    }

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    message.getGroup(1, relatedSymGroup);

    FIX::Symbol symbol;
    relatedSymGroup.get(symbol);

    const auto& userName = BCDocuments::instance()->getClientNameByID(sessionID.getTargetCompID());
    (this->*pmf)(userName, symbol);
}

/**
 * @brief Deal with the incoming quotes from clients
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find username in NewOrderSingle !" << endl;
        return;
    }

    FIX::ClOrdID orderID;
    FIX::Symbol symbol;
    FIX::OrderQty shareSize;
    FIX::OrdType orderType;
    FIX::Price price;

    message.get(orderID);
    message.get(symbol);
    message.get(shareSize);
    message.get(orderType);
    message.get(price);

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    FIX::PartyRole role;
    FIX::PartyID username;
    for (int i = 1; i <= numOfGroup; i++) {
        message.getGroup(i, idGroup);
        idGroup.get(role);
        if (role == 3) { // 3: ClientID
            idGroup.get(username);
        }
    }

    cout << "username: " << username
         << "\n\tsymbol: " << symbol
         << "\n\torderID: " << orderID
         << "\n\tprice: " << price
         << "\n\tshareSize: " << shareSize
         << "\n\torderType: " << orderType << endl;

    // Order validation
    const auto qot = Quote::ORDER_TYPE(char(orderType));

    if (username.getLength() == 0) {
        cout << "username is empty" << endl;
        return;
    } else if (CSTR_NULL == BCDocuments::instance()->getClientID(username)) {
        cout << COLOR_ERROR "This username is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (symbol.getLength() == 0) {
        cout << "symbol is empty" << endl;
        return;
    } else if (!BCDocuments::instance()->hasSymbol(symbol)) {
        cout << COLOR_ERROR "This symbol is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (orderID.getLength() == 0) {
        cout << "orderID is empty" << endl;
        return;
    }

    if (price < 0.0) {
        cout << "price is empty" << endl;
        return;
    }

    if (int(shareSize) < 0) {
        cout << "shareSize is empty" << endl;
        return;
    }

    if (::isblank(int(qot))) {
        cout << "orderType is empty" << endl;
        return;
    }

    BCDocuments::instance()->addQuoteToClientRiskManagement(username, Quote{ symbol, username, orderID, price, int(shareSize), qot });
}

/*
 * @brief Receive candle data request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::RFQRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);

    if (numOfGroup < 1) { // make sure there is a symbol in group
        cout << "Cannot find Symbol in RFQRequest" << endl;
        return;
    }

    FIX50SP2::RFQRequest::NoRelatedSym relatedSymGroup;
    message.getGroup(1, relatedSymGroup);

    FIX::Symbol symbol;
    relatedSymGroup.get(symbol);

    FIX::SubscriptionRequestType isSubscribed;
    message.get(isSubscribed);

    bool (FIXAcceptor::*pmf)(const std::string&, const std::string&) = nullptr;
    if (isSubscribed == '1') {
        pmf = &FIXAcceptor::subscribeCandleData;
    } else {
        pmf = &FIXAcceptor::unsubscribeCandleData;
    }

    const auto& userName = BCDocuments::instance()->getClientNameByID(sessionID.getTargetCompID());
    (this->*pmf)(userName, symbol);
}

void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username username; // WebClient username
    message.get(username);
    BCDocuments::instance()->registerClient(sessionID.getTargetCompID(), username); //clientID and web userName
    BCDocuments::instance()->sendHistoryToClient(username.getString());
    cout << COLOR_PROMPT "Web User [ " << username << " ] was registered." NO_COLOR << endl;
}

/* 
 * @brief Send the security list
 */
void FIXAcceptor::sendSecurityList(const std::string& userName, const std::unordered_set<std::string>& symbols)
{
    const auto& clientID = BCDocuments::instance()->getClientIDByName(userName);
    if (CSTR_NULL == clientID) {
        cout << userName << " does not exist !" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    std::set<std::string> stocks;
    std::for_each(symbols.begin(), symbols.end(), [&stocks](const auto& e) { stocks.insert(e); });
    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    for (const auto& stock : stocks) {
        relatedSymGroup.setField(FIX::Symbol(stock));
        message.addGroup(relatedSymGroup);
    }

    FIX::Session::sendToTarget(message);
}

#undef SHOW_INPUT_MSG
