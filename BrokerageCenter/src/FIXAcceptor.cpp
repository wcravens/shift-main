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

// Predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_CLEARINGBUSINESSDATE = FIX::ClearingBusinessDate("20181217");
static const auto& FIXFIELD_SYMBOL_CASH = FIX::Symbol("CASH");
static const auto& FIXFIELD_SECURITYTYPE_CASH = FIX::SecurityType(FIX::SecurityType_CASH);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);
static const auto& FIXFIELD_SECURITYTYPE_CS = FIX::SecurityType(FIX::SecurityType_COMMON_STOCK);
static const auto& FIXFIELD_QUOTESTATUS_ACCEPTED = FIX::QuoteStatus(FIX::QuoteStatus_ACCEPTED);
static const auto& FIXFIELD_EXECTYPE_ORDER_STATUS = FIX::ExecType(FIX::ExecType_ORDER_STATUS);
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType(FIX::ExecType_TRADE);
static const auto& FIXFIELD_ADVTRANSTYPE_NEW = FIX::AdvTransType(FIX::AdvTransType_NEW);
static const auto& FIXFIELD_ADVSIDE_TRADE = FIX::AdvSide(FIX::AdvSide_TRADE);
static const auto& FIXFIELD_MDUPDATEACTION_CHANGE = FIX::MDUpdateAction(FIX::MDUpdateAction_CHANGE);

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
            ::toUpper(tarID) // TODO: e.g. WEBCLIENT
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

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
#if SHOW_INPUT_MSG
    cout << endl
         << "IN: " << message << endl;
#endif
}

/**
 * @brief Events from the admin/super user
 */
void FIXAcceptor::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) // override
{
    if (FIX::MsgType_Logon != message.getHeader().getField(FIX::FIELD::MsgType))
        return;
    /* Incoming a new connection/logon from super user to BC;
        The complete actions: fromAdmin => onLogon => onMessage(UserRequest)
    */
    std::string adminName, adminPsw; // super user's info to qualify the connection
    const auto& targetID = sessionID.getTargetCompID().getValue();

    FIXT11::Logon::NoMsgTypes msgTypeGroup;
    message.getGroup(1, msgTypeGroup);
    adminName = msgTypeGroup.getField(FIX::FIELD::RefMsgType);
    message.getGroup(2, msgTypeGroup);
    adminPsw = msgTypeGroup.getField(FIX::FIELD::RefMsgType);

    const auto pswCol = (DBConnector::getInstance()->lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT password FROM traders WHERE username = '" + adminName + "';"));
    if (pswCol.size() && pswCol.front() == adminPsw) {
        // Grants the connection rights to this super user.
        cout << COLOR_PROMPT "Authentication successful for super user [" << targetID << ':' << adminName << "]." NO_COLOR << endl;
    } else {
        cout << COLOR_ERROR "ERROR: @fromAdmin(): Password of the super user [" << targetID << ':' << adminName << "] is inconsistent with the one recorded in database (in SHA1 format).\nPlease make sure the client-side send matched password to BC reliably." NO_COLOR << endl;

        throw FIX::RejectLogon(); // will disconnect the counterparty
    }
}

/**
 * @brief When super user connect to BC
 */
void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_PROMPT "\nLogon:\n[Target] " NO_COLOR << targetID << endl;

    // The following is not necessary as long as the super user also sends a onMessage(UserRequest):
    // BCDocuments::getInstance()->registerUserInDoc(targetID, ::toLower(targetID));
    // BCDocuments::getInstance()->sendHistoryToUser(::toLower(targetID));
    sendSymbols(targetID, BCDocuments::getInstance()->getSymbols());
}

/**
 * @brief When super user disconnect from BC
 */
void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;

    BCDocuments::getInstance()->unregisterTargetFromDoc(targetID); // this shall come first to timely affect other acceptor parts that are sending things aside
    BCDocuments::getInstance()->unregisterTargetFromCandles(targetID);
    BCDocuments::getInstance()->unregisterTargetFromOrderBooks(targetID);
}

/**
 * @brief General handler for incoming super & ordinal web users
 */
void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username username; // WebClient's
    message.get(username);
    BCDocuments::getInstance()->registerUserInDoc(sessionID.getTargetCompID().getValue(), username.getValue());
    BCDocuments::getInstance()->sendHistoryToUser(username.getValue());

    cout << COLOR_PROMPT "Web user [" NO_COLOR << username.getValue() << COLOR_PROMPT "] was registered.\n" NO_COLOR << endl;
}

/**
 * @brief Deal with incoming orders from clients
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups < 1) {
        cout << "Cannot find username in NewOrderSingle!" << endl;
        return;
    }

    FIX::ClOrdID orderID;
    FIX::Symbol orderSymbol;
    FIX::OrderQty orderSize;
    FIX::OrdType orderType;
    FIX::Price orderPrice;

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    FIX::PartyID username;

    message.get(orderID);
    message.get(orderSymbol);
    message.get(orderSize);
    message.get(orderType);
    message.get(orderPrice);

    message.getGroup(1, idGroup);
    idGroup.get(username);

    cout << COLOR_PROMPT "--------------------------------------\n" NO_COLOR;
    cout << "Order ID: " << orderID.getValue()
         << "\nUsername: " << username.getValue()
         << "\n\tType: " << orderType.getValue()
         << "\n\tSymbol: " << orderSymbol.getValue()
         << "\n\tSize: " << orderSize.getValue()
         << "\n\tPrice: " << orderPrice.getValue() << endl;

    // Order validation
    const auto type = static_cast<Order::Type>(static_cast<char>(orderType));
    int size = static_cast<int>(orderSize);

    if (orderID.getLength() == 0) {
        cout << "orderID is empty" << endl;
        return;
    }

    if (username.getLength() == 0) {
        cout << "username is empty" << endl;
        return;
    } else if (::STDSTR_NULL == BCDocuments::getInstance()->getTargetIDByUsername(username.getValue())) {
        cout << COLOR_ERROR "This username is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (::isblank(static_cast<int>(type))) {
        cout << "type is empty" << endl;
        return;
    }

    if (orderSymbol.getLength() == 0) {
        cout << "symbol is empty" << endl;
        return;
    } else if (!BCDocuments::getInstance()->hasSymbol(orderSymbol.getValue())) {
        cout << COLOR_ERROR "This symbol is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (size <= 0) {
        cout << "size is empty" << endl;
        return;
    }

    if (orderPrice < 0.0) {
        cout << "price is empty" << endl;
        return;
    }

    BCDocuments::getInstance()->onNewOrderForUserRiskManagement(username.getValue(), Order{ type, orderSymbol.getValue(), size, orderPrice.getValue(), orderID.getValue(), username.getValue() });
}

/*
 * @brief Receive order book subscription request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);

    if (numOfGroups < 1) {
        cout << "Cannot find Symbol in MarketDataRequest!" << endl;
        return;
    }

    FIX::SubscriptionRequestType isSubscribed;

    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    message.get(isSubscribed);

    message.getGroup(1, relatedSymGroup);
    relatedSymGroup.get(symbol);

    BCDocuments::getInstance()->manageSubscriptionInOrderBook('1' == isSubscribed.getValue(), symbol.getValue(), sessionID.getTargetCompID().getValue());
}

/*
 * @brief Receive candlestick data subscription request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::RFQRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);

    if (numOfGroups < 1) { // make sure there is a symbol in group
        cout << "Cannot find Symbol in RFQRequest" << endl;
        return;
    }

    FIX::SubscriptionRequestType isSubscribed;

    FIX50SP2::RFQRequest::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    message.get(isSubscribed);

    message.getGroup(1, relatedSymGroup);
    relatedSymGroup.get(symbol);

    BCDocuments::getInstance()->manageSubscriptionInCandlestickData('1' == isSubscribed.getValue(), symbol.getValue(), sessionID.getTargetCompID().getValue());
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
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_PositionReport));

    message.setField(FIX::PosMaintRptID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_CLEARINGBUSINESSDATE); // Required by FIX
    message.setField(::FIXFIELD_SYMBOL_CASH);
    message.setField(::FIXFIELD_SECURITYTYPE_CASH);
    message.setField(FIX::PriceDelta(summary.getTotalPL()));

    FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
    usernameGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
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
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_PositionReport));

    message.setField(FIX::PosMaintRptID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_CLEARINGBUSINESSDATE); // Required by FIX
    message.setField(FIX::Symbol(item.getSymbol()));
    message.setField(::FIXFIELD_SECURITYTYPE_CS);
    message.setField(FIX::SettlPrice(item.getLongPrice()));
    message.setField(FIX::PriorSettlPrice(item.getShortPrice()));
    message.setField(FIX::PriceDelta(item.getPL()));

    FIX50SP2::PositionReport::NoPartyIDs usernameGroup;
    usernameGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    usernameGroup.setField(FIX::PartyID(username));
    message.addGroup(usernameGroup);

    FIX50SP2::PositionReport::NoPositions qtyGroup;
    qtyGroup.setField(FIX::LongQty(item.getLongShares()));
    qtyGroup.setField(FIX::ShortQty(item.getShortShares()));
    message.addGroup(qtyGroup);

    FIX::Session::sendToTarget(message);
}

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

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MassQuoteAcknowledgement));

    message.setField(::FIXFIELD_QUOTESTATUS_ACCEPTED); // Required by FIX
    message.setField(FIX::Account(username));

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets orderSetGroup;
    for (const auto& kv : orders) {
        auto& order = kv.second;
        orderSetGroup.setField(FIX::QuoteSetID(order.getID()));
        orderSetGroup.setField(FIX::UnderlyingSymbol(order.getSymbol()));
        orderSetGroup.setField(FIX::UnderlyingOptAttribute(order.getType()));
        orderSetGroup.setField(FIX::UnderlyingQty(order.getSize()));
        orderSetGroup.setField(FIX::UnderlyingPx(order.getPrice()));
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
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    if (report.orderStatus == Order::Status::NEW || report.orderStatus == Order::Status::PENDING_CANCEL || report.orderStatus == Order::Status::REJECTED) {
        message.setField(::FIXFIELD_EXECTYPE_ORDER_STATUS); // Required by FIX
    } else {
        message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    }
    message.setField(FIX::OrdStatus(report.orderStatus));
    message.setField(FIX::Symbol(report.orderSymbol));
    message.setField(FIX::Side(report.orderType)); // FIXME: separate Side and OrdType
    message.setField(FIX::OrdType(report.orderType)); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(report.orderPrice));
    message.setField(FIX::EffectiveTime(report.execTime, 6));
    message.setField(FIX::LastMkt(report.destination));
    message.setField(FIX::LeavesQty(report.currentSize));
    message.setField(FIX::CumQty(report.executedSize));
    message.setField(FIX::TransactTime(report.serverTime, 6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
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
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_Advertisement));

    message.setField(FIX::AdvId(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_ADVTRANSTYPE_NEW);
    message.setField(FIX::Symbol(transac.symbol));
    message.setField(::FIXFIELD_ADVSIDE_TRADE);
    message.setField(FIX::Quantity(transac.size));
    message.setField(FIX::Price(transac.price));
    message.setField(FIX::TransactTime(transac.simulationTime, 6)); // TODO: use simulationTime (execTime) and realTime (serverTime) everywhere
    message.setField(FIX::LastMkt(transac.destination));

    for (const auto& targetID : BCDocuments::getInstance()->getTargetList()) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/**
 * @brief Send complete order book by type
 */
void FIXAcceptor::sendOrderBook(const std::vector<std::string>& targetList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataSnapshotFullRefresh));

    message.setField(FIX::Symbol(orderBookName.begin()->second.begin()->second.getSymbol()));

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    const auto obt = orderBookName.begin()->second.begin()->second.getType();

    if (obt == OrderBookEntry::Type::GLB_BID || obt == OrderBookEntry::Type::LOC_BID) { // reverse the global/local bid order book order
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

    for (const auto& targetID : targetList) {
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
    entryGroup.setField(FIX::MDMkt(entry.getDestination()));

    message.addGroup(entryGroup);
}

/**
 * @brief   Send order book update to one client
 */
void FIXAcceptor::sendOrderBookUpdate(const std::vector<std::string>& targetList, const OrderBookEntry& update)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATEACTION_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(static_cast<char>(update.getType())));
    entryGroup.setField(FIX::Symbol(update.getSymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(update.getSize()));
    entryGroup.setField(FIX::MDEntryDate(update.getDate()));
    entryGroup.setField(FIX::MDEntryTime(update.getTime()));
    entryGroup.setField(FIX::MDMkt(update.getDestination()));

    message.addGroup(entryGroup);

    for (const auto& targetID : targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

void FIXAcceptor::sendCandlestickData(const std::vector<std::string>& targetList, const CandlestickDataPoint& cdPoint)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityStatus));

    message.setField(FIX::Symbol(cdPoint.getSymbol()));
    message.setField(FIX::Text(std::to_string(cdPoint.getTimeFrom()))); // no need for timestamp
    message.setField(FIX::StrikePrice(cdPoint.getOpenPrice())); // Open price
    message.setField(FIX::HighPx(cdPoint.getHighPrice())); // High price
    message.setField(FIX::LowPx(cdPoint.getLowPrice())); // Low price
    message.setField(FIX::LastPx(cdPoint.getClosePrice())); // Close price

    for (const auto& targetID : targetList) {
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

/*
 * @brief Send the security list
 */
void FIXAcceptor::sendSymbols(const std::string& targetID, const std::unordered_set<std::string>& symbols)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
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
