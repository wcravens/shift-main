/*
** This file contains the implementation of functions in FIXAcceptor.h
**/

#include "FIXAcceptor.h"

#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>

#define SHOW_INPUT_MSG false

/* static */ std::string FIXAcceptor::s_senderID;

// Predefined constant FIX message (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1");
static const auto& FIXFIELD_MSGTY_MASSQUOTEACK = FIX::MsgType(FIX::MsgType_MassQuoteAcknowledgement);
static const auto& FIXFIELD_QUOTESTAT = FIX::QuoteStatus(0); // 0 = Accepted
static const auto& FIXFIELD_TXT_WL = FIX::Text("waitingList");
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

    const auto targetIDs = shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT username FROM traders WHERE super = TRUE;");
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
 * @brief Send Portfolio Summary to LC
 */
void FIXAcceptor::sendPortfolioSummary(const std::string& username, const PortfolioSummary& summary)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
    if (::STDSTR_NULL == targetID) {
        cout << "sendPortfolioSummary(): ";
        cout << username << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

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

    FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
    usernameGroup.setField(::FIXFIELD_CLIENTID);
    usernameGroup.setField(FIX::PartyID(username));
    message.addGroup(usernameGroup);

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
 * @brief Send Portfolio Item to LC
 */
void FIXAcceptor::sendPortfolioItem(const std::string& username, const PortfolioItem& item)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
    if (::STDSTR_NULL == targetID) {
        cout << "sendPortfolioItem(): ";
        cout << username << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

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

    FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
    usernameGroup.setField(::FIXFIELD_CLIENTID);
    usernameGroup.setField(FIX::PartyID(username));
    message.addGroup(usernameGroup);

    FIX50SP2::PositionReport::NoPositions qtyGroup;
    qtyGroup.setField(FIX::LongQty(item.getLongShares()));
    qtyGroup.setField(FIX::ShortQty(item.getShortShares()));
    message.addGroup(qtyGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief
 */
void FIXAcceptor::sendWaitingList(const std::string& username, const std::unordered_map<std::string, Order>& orders)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
    if (::STDSTR_NULL == targetID) {
        cout << "sendWaitingList(): ";
        cout << username << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX50SP2::MassQuoteAcknowledgement message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(::FIXFIELD_MSGTY_MASSQUOTEACK);

    message.setField(::FIXFIELD_QUOTESTAT); // Required by FIX
    message.setField(::FIXFIELD_TXT_WL);
    message.setField(FIX::Account(username));

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets orderSetGroup;
    for (const auto& i : orders) {
        auto& order = i.second;
        orderSetGroup.setField(FIX::QuoteSetID(targetID));
        orderSetGroup.setField(FIX::UnderlyingSecurityID(order.getOrderID()));
        orderSetGroup.setField(FIX::UnderlyingSymbol(order.getSymbol()));
        orderSetGroup.setField(FIX::UnderlyingStrikePrice(order.getPrice()));
        orderSetGroup.setField(FIX::UnderlyingSymbolSfx(std::to_string(order.getShareSize()))); // FIXME: replaceb by other field
        orderSetGroup.setField(FIX::UnderlyingIssuer(std::to_string(order.getOrderType())));
        message.addGroup(orderSetGroup);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Sending the order confirmation to the client,
 * because report.status usual set to 1,
 * Then the client will notify it is a confirmation
 */
void FIXAcceptor::sendConfirmationReport(const Report& report)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(report.username);
    if (::STDSTR_NULL == targetID) {
        cout << "sendConfirmationReport(): ";
        cout << report.username << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID));
    message.setField(FIX::ClOrdID(report.username));
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
    idGroup.setField(FIX::PartyID(report.username));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send the last stock price to LC
 * @param transac as a Transaction object to provide last price
 */
void FIXAcceptor::sendLastPrice2All(const Transaction& transac)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
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

    for (const auto& kv : BCDocuments::getInstance()->getConnectedTargetIDsMap()) {
        header.setField(FIX::TargetCompID(kv.first));
        FIX::Session::sendToTarget(message);
    }
}

/**
 * @brief Send complete order book by type
 */
void FIXAcceptor::sendOrderBook(const std::vector<std::string>& userList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataSnapshotFullRefresh));

    message.setField(FIX::Symbol(orderBookName.begin()->second.begin()->second.getSymbol()));

    using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    const auto obt = orderBookName.begin()->second.begin()->second.getType();

    if (obt == OBT::GLB_BID || obt == OBT::LOC_BID) { //reverse the Bid/bid order book order
        for (auto ri = orderBookName.crbegin(); ri != orderBookName.crend(); ++ri) {
            for (const auto& j : ri->second)
                s_setAddGroupIntoOrderAckMsg(message, entryGroup, j.second);
        }
    } else { // *_ASK
        for (const auto& i : orderBookName) {
            for (const auto& j : i.second)
                s_setAddGroupIntoOrderAckMsg(message, entryGroup, j.second);
        }
    }

    for (const auto& username : userList) {
        const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
        if (::STDSTR_NULL == targetID) {
            continue;
        }
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/*static*/ inline void FIXAcceptor::s_setAddGroupIntoOrderAckMsg(FIX::Message& message, FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries& entryGroup, const OrderBookEntry& entry)
{
    entryGroup.setField(FIX::MDEntryType((char)entry.getType()));
    entryGroup.setField(FIX::MDEntryPx(entry.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(entry.getSize()));
    entryGroup.setField(FIX::MDEntryDate(entry.getDate()));
    entryGroup.setField(FIX::MDEntryTime(entry.getTime()));

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(entry.getDestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);
}

/**
 * @brief   Send order book update to one client
 * @param   userList as a vector to provide the name of clients
 * @param   update as an OrderBookEntry object to provide price and others
 */
void FIXAcceptor::sendOrderBookUpdate(const std::vector<std::string>& userList, const OrderBookEntry& update)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATE_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(char(update.getType())));
    entryGroup.setField(FIX::Symbol(update.getSymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(update.getSize()));
    entryGroup.setField(FIX::MDEntryDate(update.getDate()));
    entryGroup.setField(FIX::MDEntryTime(update.getTime()));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(::FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(update.getDestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);

    for (const auto& username : userList) {
        const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
        if (::STDSTR_NULL == targetID) {
            continue;
        }
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

void FIXAcceptor::sendCandlestickData(const std::vector<std::string>& userList, const CandlestickDataPoint& cdPoint)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityStatus));

    message.setField(FIX::Symbol(cdPoint.getSymbol()));
    message.setField(FIX::Text(std::to_string(cdPoint.getTimeFrom()))); // no need for timestamp
    message.setField(FIX::StrikePrice(cdPoint.getOpenPrice())); // Open price
    message.setField(FIX::HighPx(cdPoint.getHighPrice())); // High price
    message.setField(FIX::LowPx(cdPoint.getLowPrice())); // Low price
    message.setField(FIX::LastPx(cdPoint.getClosePrice())); // Close price

    for (const auto& username : userList) {
        const auto& targetID = BCDocuments::getInstance()->getTargetIDByUsername(username);
        if (::STDSTR_NULL == targetID) {
            continue;
        }
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/**
 * @brief Method called when a new Session is created.
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
    const auto& username = BCDocuments::getInstance()->getUsernameByTargetID(targetID); // TODO
    cout << COLOR_PROMPT "\nLogon:\n[Target] " NO_COLOR << targetID << endl;
    if (::STDSTR_NULL == username) {
        cout << "onLogon(): ";
        cout << targetID << " does not have an associated admin!" << endl;
        return;
    }

    // send security list of all stocks to user when he login
    sendSymbols(targetID, BCDocuments::getInstance()->getSymbols());

    BCDocuments::getInstance()->sendHistoryToUser(username);
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    auto& targetID = sessionID.getTargetCompID();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;

    const auto& username = BCDocuments::getInstance()->getUsernameByTargetID(targetID);
    if (::STDSTR_NULL == username) {
        cout << "onLogon(): ";
        cout << targetID << " does not have an associated admin!" << endl;
        return;
    }
    cout << COLOR_WARNING "[User] " NO_COLOR << username << endl;
    BCDocuments::getInstance()->unregisterUserInDoc(username); // this shall come first to timely affect other acceptor parts that are sending things aside
    BCDocuments::getInstance()->removeUserFromCandles(username);
    BCDocuments::getInstance()->removeUserFromOrderBooks(username);
    cout << COLOR_WARNING "[Session] " NO_COLOR << sessionID << '\n'
         << endl;
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

    const auto pswCol = (DBConnector::getInstance()->lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT password FROM traders WHERE username = '" + adminName + "';"));
    if (pswCol.size() && pswCol.front() == adminPsw) {
        BCDocuments::getInstance()->registerUserInDoc(adminName, targetID);
        cout << COLOR_PROMPT "Authentication successful for " << targetID << ':' << adminName << NO_COLOR << endl;
    } else {
        cout << COLOR_WARNING "Username or password was wrong for " << targetID << ':' << adminName << NO_COLOR << endl;
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

void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username username; // WebClient username
    message.get(username);
    BCDocuments::getInstance()->registerWebUserInDoc(username, sessionID.getTargetCompID());
    BCDocuments::getInstance()->sendHistoryToUser(username.getString());
    cout << COLOR_PROMPT "Web user [ " NO_COLOR << username << COLOR_PROMPT " ] was registered.\n" NO_COLOR << endl;
}

/**
 * @brief Deal with incoming orders from clients
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
        if (::FIXFIELD_CLIENTID == role) {
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
    const auto qot = Order::ORDER_TYPE(char(orderType));

    if (username.getLength() == 0) {
        cout << "username is empty" << endl;
        return;
    } else if (::STDSTR_NULL == BCDocuments::getInstance()->getTargetIDByUsername(username)) {
        cout << COLOR_ERROR "This username is not register in the Brokerage Center" NO_COLOR << endl;
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

    if (int(shareSize) <= 0) {
        cout << "shareSize is empty" << endl;
        return;
    }

    if (::isblank(int(qot))) {
        cout << "orderType is empty" << endl;
        return;
    }

    BCDocuments::getInstance()->addOrderToUserRiskManagement(username, Order{ symbol, username, orderID, price, int(shareSize), qot });
}

/*
 * @brief Receive order book subscription request from LC
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

    const auto& username = BCDocuments::getInstance()->getUsernameByTargetID(sessionID.getTargetCompID());
    BCDocuments::getInstance()->manageUsersInOrderBook('1' == isSubscribed, symbol, username);
}

/*
 * @brief Receive candlestick data subscription request from LC
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

    const auto& username = BCDocuments::getInstance()->getUsernameByTargetID(sessionID.getTargetCompID());
    BCDocuments::getInstance()->manageUsersInCandlestickData('1' == isSubscribed, username, symbol);
}

/*
 * @brief Send the security list
 */
void FIXAcceptor::sendSymbols(const std::string& targetID, const std::unordered_set<std::string>& symbols)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    // make ascending ordered stock symbols
    std::vector<std::string> ascSymbols(symbols.begin(), symbols.end());
    std::sort(ascSymbols.begin(), ascSymbols.end());

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;

    for (const auto& symbol : ascSymbols) {
        relatedSymGroup.setField(FIX::Symbol(symbol));
        message.addGroup(relatedSymGroup);
    }

    FIX::Session::sendToTarget(message);
}

#undef SHOW_INPUT_MSG
