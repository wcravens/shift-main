/*
** This file contains the implementation of functions in FIXAcceptor.h
**/

#include "FIXAcceptor.h"

#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
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
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType('F'); // F = Trade
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0); // Quantity open for further execution
static const auto& FIXFIELD_MSGTY_POSIREPORT = FIX::MsgType(FIX::MsgType_PositionReport);

FIXAcceptor::~FIXAcceptor() // override
{
    disconnectClientComputers();
}

/*static*/ FIXAcceptor* FIXAcceptor::getInstance()
{
    static FIXAcceptor s_FIXAccInst;
    return &s_FIXAccInst;
}

void FIXAcceptor::connectClientComputers(const std::string& configFile, bool verbose)
{
    disconnectClientComputers();

    const auto targetIDs = DBConnector::s_readRowsOfField("SELECT username FROM traders WHERE super = TRUE;");
    if (targetIDs.empty()) {
        cout << COLOR_WARNING "WARNING: No target is present in the database. The system terminates." NO_COLOR << endl;
        std::terminate();
    }

    FIX::SessionSettings settings(configFile);
    const FIX::Dictionary& commonDict = settings.get();

    for (const auto& tarID : targetIDs) {
        FIX::SessionID sid(commonDict.getString("BeginString"), commonDict.getString("SenderCompID") // e.g. BROKERAGECENTER
            ,
            ::toUpper(tarID) // e.g. WEBCLIENT
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

    m_acceptorPtr.reset(new FIX::SocketAcceptor(*getInstance(), *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Acceptor is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_acceptorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXAcceptor::disconnectClientComputers()
{
    if (!m_acceptorPtr)
        return;

    cout << '\n'
         << COLOR "Acceptor is stopping..." NO_COLOR << '\n'
         << flush;

    m_acceptorPtr->stop();
    m_acceptorPtr = nullptr;
    m_messageStoreFactoryPtr = nullptr;
    m_logFactoryPtr = nullptr;
}

/**
 * @brief Send Portfolio Item to LC
 */
void FIXAcceptor::sendPortfolioItem(const std::string& userName, const std::string& targetID, const PortfolioItem& item)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIXFIELD_MSGTY_POSIREPORT);

    message.setField(FIX::PosMaintRptID(shift::crossguid::newGuid().str()));
    message.setField(FIX::ClearingBusinessDate("20181102")); // Required by FIX
    message.setField(FIX::Symbol(item.getSymbol()));
    message.setField(FIX::SecurityType("CS"));
    message.setField(FIX::SettlPrice(item.getLongPrice()));
    message.setField(FIX::PriorSettlPrice(item.getShortPrice()));
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
void FIXAcceptor::sendPortfolioSummary(const std::string& userName, const std::string& targetID, const PortfolioSummary& summary)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
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
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        cout << "sendQuoteHistory(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX50SP2::MassQuoteAcknowledgement message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(::FIXFIELD_MSGTY_MASSQUOTEACK);

    message.setField(::FIXFIELD_QUOTESTAT); // Required by FIX
    message.setField(::FIXFIELD_TXT_QH);
    message.setField(FIX::Account(userName));

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets quoteSetGroup;
    for (const auto& i : quotes) {
        auto& quote = i.second;
        quoteSetGroup.setField(FIX::QuoteSetID(targetID));
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
 * @param   userName as a string to provide the name of client user
 * @param   update as an OrderBook object to provide price and others
 */
void FIXAcceptor::sendOrderbookUpdate(const std::string& userName, const OrderBookEntry& update)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        cout << "sendOrderbookUpdate(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATE_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(char(update.getType())));
    entryGroup.setField(FIX::Symbol(update.getSymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(update.getSize()));
    entryGroup.setField(FIX::Text(std::to_string(update.getTime())));
    entryGroup.setField(FIX::ExpireTime(update.getUtcTime(), 6));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(::FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(update.getDestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief   Send order book update to all clients
 * @param   userList as a list to provide the name of clients
 * @param   update as an OrderBook object to provide price and others
 */
void FIXAcceptor::sendOrderbookUpdate2All(const std::unordered_set<std::string>& userList, const OrderBookEntry& update)
{
    for (const auto& userName : userList) {
        sendOrderbookUpdate(userName, update);
    }
}

/**
 * @brief Send order book update to all clients
 */
void FIXAcceptor::sendNewBook2all(const std::unordered_set<std::string>& userList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    for (const auto& userName : userList) {
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
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(report.userName);
    if (CSTR_NULL == targetID) {
        cout << "sendConfirmationReport(): ";
        cout << report.userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID));
    message.setField(FIX::ClOrdID(report.userName));
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
    idGroup.setField(FIX::PartyID(report.userName));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

static void s_setAddGroupIntoQuoteAckMsg(FIX::Message& message, FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries& entryGroup, const OrderBookEntry& odrBk)
{
    entryGroup.setField(FIX::MDEntryType((char)odrBk.getType()));
    entryGroup.setField(FIX::MDEntryPx(odrBk.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(odrBk.getSize()));
    entryGroup.setField(FIX::Text(std::to_string(odrBk.getTime())));

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(odrBk.getDestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);
}

/**
 * @brief Send complete order book by type
 */
void FIXAcceptor::sendOrderBook(const std::string& userName, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        cout << "sendOrderBook(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataSnapshotFullRefresh));

    message.setField(FIX::Symbol(orderBookName.begin()->second.begin()->second.getSymbol()));

    using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    const auto obt = orderBookName.begin()->second.begin()->second.getType();

    if (obt == OBT::GLB_BID || obt == OBT::LOC_BID) { //reverse the Bid/bid orderbook order
        for (auto ri = orderBookName.crbegin(); ri != orderBookName.crend(); ++ri) {
            for (const auto& j : ri->second)
                ::s_setAddGroupIntoQuoteAckMsg(message, entryGroup, j.second);
        }
    } else { // *_ASK
        for (const auto& i : orderBookName) {
            for (const auto& j : i.second)
                ::s_setAddGroupIntoQuoteAckMsg(message, entryGroup, j.second);
        }
    }

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::sendTempCandlestickData(const std::string& userName, const TempCandlestickData& tmpCandle)
{ //for candlestick data
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        cout << "sendTempCandlestickData(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityStatus));

    message.setField(FIX::Symbol(tmpCandle.getSymbol()));
    message.setField(FIX::Text(std::to_string(tmpCandle.getTempTimeFrom()))); // no need for timestamp
    message.setField(FIX::StrikePrice(tmpCandle.getTempOpenPrice())); // Open price
    message.setField(FIX::HighPx(tmpCandle.getTempHighPrice())); // High price
    message.setField(FIX::LowPx(tmpCandle.getTempLowPrice())); // Low price
    message.setField(FIX::LastPx(tmpCandle.getTempClosePrice())); // Close price

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
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        cout << "sendLatestStockPrice(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
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
    for (const auto& i : BCDocuments::getInstance()->getUserList()) {
        sendLatestStockPrice(i.first, transac);
    }
}

inline bool FIXAcceptor::subscribeOrderBook(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::getInstance()->manageStockOrderBookUser(true, symbol, userName);
}

inline bool FIXAcceptor::unsubscribeOrderBook(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::getInstance()->manageStockOrderBookUser(false, symbol, userName);
}

inline bool FIXAcceptor::subscribeCandleData(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::getInstance()->manageCandleStickDataUser(true, userName, symbol);
}

inline bool FIXAcceptor::unsubscribeCandleData(const std::string& userName, const std::string& symbol)
{
    return BCDocuments::getInstance()->manageCandleStickDataUser(false, userName, symbol);
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
    auto& targetID = sessionID.getTargetCompID();
    const auto& userName = BCDocuments::getInstance()->getUserNameByTargetID(targetID); // TODO
    cout << COLOR_PROMPT "\nLogon:\n[Target] " NO_COLOR << targetID << endl;
    if (CSTR_NULL == targetID) {
        cout << "onLogon(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    // send security list of all stocks to user when he login
    sendSecurityList(targetID, BCDocuments::getInstance()->getSymbols());

    BCDocuments::getInstance()->sendHistoryToUser(userName);
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    auto& targetID = sessionID.getTargetCompID();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;
    if (::toUpper(targetID) == "WEBCLIENTID")
        return;

    const auto& userName = BCDocuments::getInstance()->getUserNameByTargetID(targetID);
    if (CSTR_NULL == targetID) {
        cout << "onLogout(): ";
        cout << userName << " does not exist: Target computer ID not identified!" << endl;
        return;
    }
    cout << COLOR_WARNING "[User] " NO_COLOR << userName << endl;
    // TODO
    BCDocuments::getInstance()->removeUserFromCandles(userName);
    // cout << "DEBUG MUTEX: removeUserFromCandles" << endl;
    BCDocuments::getInstance()->unregisterUserInDoc(userName);
    // cout << "DEBUG MUTEX: unregisterUserInDoc" << endl;
    BCDocuments::getInstance()->removeUserFromStocks(userName);
    // cout << "DEBUG MUTEX: removeUserFromStocks" << endl;
    cout << COLOR_WARNING "[Session] " NO_COLOR << sessionID << '\n'
         << endl;
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

    std::string adminName, adminPsw;
    const auto& targetID = static_cast<std::string>(sessionID.getTargetCompID());

    FIXT11::Logon::NoMsgTypes msgTypeGroup;
    message.getGroup(1, msgTypeGroup);
    adminName = msgTypeGroup.getField(FIX::FIELD::RefMsgType);

    message.getGroup(2, msgTypeGroup);
    adminPsw = msgTypeGroup.getField(FIX::FIELD::RefMsgType);

    const auto pswCol = DBConnector::s_readRowsOfField("SELECT password FROM traders WHERE username = '" + adminName + "';");
    if (pswCol.size() && pswCol.front() == adminPsw) {
        BCDocuments::getInstance()->registerUserInDoc(targetID, adminName);
        cout << COLOR_PROMPT "Authentication successful for " << targetID << ':' << adminName << NO_COLOR << endl;
    } else {
        // TODO, send message
        FIX::Message tokenMessage;
        FIXT11::Logon::NoMsgTypes msgTypeGroup;
        msgTypeGroup.set(FIX::RefMsgType("m_username"));
        tokenMessage.addGroup(msgTypeGroup);
        msgTypeGroup.set(FIX::RefMsgType("m_password"));
        tokenMessage.addGroup(msgTypeGroup);

        cout << COLOR_WARNING "User name or password was wrong for " << targetID << ':' << adminName << NO_COLOR << endl;
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

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    message.getGroup(1, relatedSymGroup);

    FIX::Symbol symbol;
    relatedSymGroup.get(symbol);

    const auto& userName = BCDocuments::getInstance()->getUserNameByTargetID(sessionID.getTargetCompID());
    if ('1' == isSubscribed) {
        subscribeOrderBook(userName, symbol);
    } else {
        unsubscribeOrderBook(userName, symbol);
    }
}

/**
 * @brief Deal with the incoming quotes from clients
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find userName in NewOrderSingle !" << endl;
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
    FIX::PartyID userName;
    for (int i = 1; i <= numOfGroup; i++) {
        message.getGroup(i, idGroup);
        idGroup.get(role);
        if (::FIXFIELD_CLIENTID == role) {
            idGroup.get(userName);
        }
    }

    cout << "userName: " << userName
         << "\n\tsymbol: " << symbol
         << "\n\torderID: " << orderID
         << "\n\tprice: " << price
         << "\n\tshareSize: " << shareSize
         << "\n\torderType: " << orderType << endl;

    // Order validation
    const auto qot = Quote::ORDER_TYPE(char(orderType));

    if (userName.getLength() == 0) {
        cout << "userName is empty" << endl;
        return;
    } else if (CSTR_NULL == BCDocuments::getInstance()->getTargetID(userName)) {
        cout << COLOR_ERROR "This userName is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (symbol.getLength() == 0) {
        cout << "symbol is empty" << endl;
        return;
    } else if (!BCDocuments::getInstance()->hasSymbol(symbol)) {
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

    BCDocuments::getInstance()->addQuoteToUserRiskManagement(userName, Quote{ symbol, userName, orderID, price, int(shareSize), qot });
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

    const auto& userName = BCDocuments::getInstance()->getUserNameByTargetID(sessionID.getTargetCompID());
    if ('1' == isSubscribed) {
        subscribeCandleData(userName, symbol);
    } else {
        unsubscribeCandleData(userName, symbol);
    }
}

void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username userName; // WebClient userName
    message.get(userName);
    BCDocuments::getInstance()->registerUserInDoc(sessionID.getTargetCompID(), userName);
    BCDocuments::getInstance()->sendHistoryToUser(userName.getString());
    cout << COLOR_PROMPT "Web user [ " NO_COLOR << userName << COLOR_PROMPT " ] was registered.\n" NO_COLOR << endl;
}

/*
 * @brief Send the security list
 */
void FIXAcceptor::sendSecurityList(const std::string& targetID, const std::unordered_set<std::string>& symbols)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    // make acscending ordered stock symbols
    std::vector<std::string> ascStocks(symbols.begin(), symbols.end());
    std::sort(ascStocks.begin(), ascStocks.end());

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;

    for (const auto& stock : ascStocks) {
        relatedSymGroup.setField(FIX::Symbol(stock));
        message.addGroup(relatedSymGroup);
    }

    FIX::Session::sendToTarget(message);
}

#undef SHOW_INPUT_MSG
