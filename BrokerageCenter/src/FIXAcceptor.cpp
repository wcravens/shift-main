/*
** This file contains the implementation of functions in FIXAcceptor.h
**/

#include "FIXAcceptor.h"

#include "BCDocuments.h"
#include "DBConnector.h"

#include <atomic>
#include <cassert>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>

#define SHOW_FROMAPP_MSG false

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
    disconnectClients();
}

/*static*/ FIXAcceptor* FIXAcceptor::getInstance()
{
    static FIXAcceptor s_FIXAccInst;
    return &s_FIXAccInst;
}

void FIXAcceptor::connectClients(const std::string& configFile, bool verbose)
{
    disconnectClients();

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

void FIXAcceptor::disconnectClients()
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
                s_setAddGroupIntoMarketDataMsg(message, entryGroup, j.second);
        }
    } else { // *_ASK
        for (const auto& i : orderBookName) {
            for (const auto& j : i.second)
                s_setAddGroupIntoMarketDataMsg(message, entryGroup, j.second);
        }
    }

    for (const auto& targetID : targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/*static*/ inline void FIXAcceptor::s_setAddGroupIntoMarketDataMsg(FIX::Message& message, FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries& entryGroup, const OrderBookEntry& entry)
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
    entryGroup.setField(FIX::MDEntryType(update.getType()));
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
 * @brief Sending the order confirmation to the client,
 * because report.status usual set to 1,
 * Then the client will notify it is a confirmation
 */
void FIXAcceptor::sendConfirmationReport(const Report& report)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserID(report.userID);
    if (::STDSTR_NULL == targetID) {
        cout << "sendConfirmationReport(): ";
        cout << report.userID << " does not exist. Target computer ID not identified!" << endl;
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
    idGroup.setField(FIX::PartyID(report.userID));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send Portfolio Summary to LC
 */
void FIXAcceptor::sendPortfolioSummary(const std::string& userID, const PortfolioSummary& summary)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserID(userID);
    if (::STDSTR_NULL == targetID) {
        cout << "sendPortfolioSummary(): ";
        cout << userID << " does not exist: Target computer ID not identified!" << endl;
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

    FIX50SP2::PositionReport::NoPartyIDs userIDGroup;
    userIDGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    userIDGroup.setField(FIX::PartyID(userID));
    message.addGroup(userIDGroup);

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
void FIXAcceptor::sendPortfolioItem(const std::string& userID, const PortfolioItem& item)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserID(userID);
    if (::STDSTR_NULL == targetID) {
        cout << "sendPortfolioItem(): ";
        cout << userID << " does not exist: Target computer ID not identified!" << endl;
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

    FIX50SP2::PositionReport::NoPartyIDs userIDGroup;
    userIDGroup.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    userIDGroup.setField(FIX::PartyID(userID));
    message.addGroup(userIDGroup);

    FIX50SP2::PositionReport::NoPositions qtyGroup;
    qtyGroup.setField(FIX::LongQty(item.getLongShares()));
    qtyGroup.setField(FIX::ShortQty(item.getShortShares()));
    message.addGroup(qtyGroup);

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::sendWaitingList(const std::string& userID, const std::unordered_map<std::string, Order>& orders)
{
    const auto& targetID = BCDocuments::getInstance()->getTargetIDByUserID(userID);
    if (::STDSTR_NULL == targetID) {
        cout << "sendWaitingList(): ";
        cout << userID << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX50SP2::MassQuoteAcknowledgement message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MassQuoteAcknowledgement));

    message.setField(::FIXFIELD_QUOTESTATUS_ACCEPTED); // Required by FIX
    message.setField(FIX::Account(userID));

    FIX50SP2::MassQuoteAcknowledgement::NoQuoteSets orderSetGroup;
    for (const auto& kv : orders) {
        auto& order = kv.second;
        orderSetGroup.setField(FIX::QuoteSetID(order.getID()));
        orderSetGroup.setField(FIX::UnderlyingSymbol(order.getSymbol()));
        orderSetGroup.setField(FIX::UnderlyingOptAttribute(order.getType())); // TODO: incorrect field (require new message type)
        orderSetGroup.setField(FIX::UnderlyingQty(order.getSize()));
        orderSetGroup.setField(FIX::UnderlyingPx(order.getPrice()));
        orderSetGroup.setField(FIX::UnderlyingAdjustedQuantity(order.getExecutedSize()));
        orderSetGroup.setField(FIX::UnderlyingFXRateCalc(order.getStatus())); // TODO: incorrect field (require new message type)
        message.addGroup(orderSetGroup);
    }

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send the security list
 */
void FIXAcceptor::sendSecurityList(const std::string& targetID, const std::unordered_set<std::string>& symbols)
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

void FIXAcceptor::sendUserIDResponse(const std::string& targetID, const std::string& username, const std::string& userID)
{
    FIX::Message msgRes;

    FIX::Header& header = msgRes.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserResponse));

    msgRes.setField(FIX::Username(username));
    msgRes.setField(FIX::UserRequestID(userID));

    FIX::Session::sendToTarget(msgRes);
}

/**
 * @brief Method called when a new Session is created.
 * Set Sender and Target Comp ID.
 */
void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    /* SenderID: BROKERAGECENTER */
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
    sendSecurityList(targetID, BCDocuments::getInstance()->getSymbols());
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

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
#if SHOW_FROMAPP_MSG
    synchPrint("\nfromAPP message: " + message + '\n');
#endif
}

/**
 * @brief General handler for incoming super & ordinal web users. After this point, usernames are represented by corresponding userIDs.
 */
void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username username;
    message.get(username);
    // not used:
    FIX::UserRequestID userReqID;
    message.get(userReqID);

    const auto idCol = (DBConnector::getInstance()->lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT id FROM traders WHERE username = '" + username.getValue() + "';"));
    if (idCol.empty()) {
        cout << COLOR_ERROR "ERROR: Cannot retrieve id for user [" << username.getValue() << "]. Please check this user that is already added into 'traders'." NO_COLOR << endl;
        return;
    }
    const auto& userID = idCol[0];

    sendUserIDResponse(sessionID.getTargetCompID().getValue(), username, userID);

    BCDocuments::getInstance()->registerUserInDoc(sessionID.getTargetCompID().getValue(), userID);
    BCDocuments::getInstance()->sendHistoryToUser(userID);

    synchPrint(COLOR_PROMPT "Web user [" NO_COLOR + username.getValue() + COLOR_PROMPT "](" NO_COLOR + userID + COLOR_PROMPT ") was registered.\n\n" NO_COLOR);
}

/*
 * @brief Receive order book subscription request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find Symbol in MarketDataRequest!" << endl;
        return;
    }

    static FIX::SubscriptionRequestType isSubscribed;

    static FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SubscriptionRequestType* pIsSubscribed;

    FIX50SP2::MarketDataRequest::NoRelatedSym* pRelatedSymGroup;
    FIX::Symbol* pSymbol;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pIsSubscribed = &isSubscribed;
        pRelatedSymGroup = &relatedSymGroup;
        pSymbol = &symbol;
    } else { // > 1 threads; always safe way:
        pIsSubscribed = new decltype(isSubscribed);
        pRelatedSymGroup = new decltype(relatedSymGroup);
        pSymbol = new decltype(symbol);
    }

    message.get(*pIsSubscribed);

    message.getGroup(1, *pRelatedSymGroup);
    pRelatedSymGroup->get(*pSymbol);

    BCDocuments::getInstance()->manageSubscriptionInOrderBook('1' == pIsSubscribed->getValue(), pSymbol->getValue(), sessionID.getTargetCompID().getValue());

    if (prevCnt) { // > 1 threads
        delete pIsSubscribed;
        delete pRelatedSymGroup;
        delete pSymbol;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/*
 * @brief Receive candlestick data subscription request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::RFQRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) { // make sure there is a symbol in group
        cout << "Cannot find Symbol in RFQRequest" << endl;
        return;
    }

    static FIX::SubscriptionRequestType isSubscribed;

    static FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SubscriptionRequestType* pIsSubscribed;

    FIX50SP2::MarketDataRequest::NoRelatedSym* pRelatedSymGroup;
    FIX::Symbol* pSymbol;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pIsSubscribed = &isSubscribed;
        pRelatedSymGroup = &relatedSymGroup;
        pSymbol = &symbol;
    } else { // > 1 threads; always safe way:
        pIsSubscribed = new decltype(isSubscribed);
        pRelatedSymGroup = new decltype(relatedSymGroup);
        pSymbol = new decltype(symbol);
    }

    message.get(*pIsSubscribed);

    message.getGroup(1, *pRelatedSymGroup);
    pRelatedSymGroup->get(*pSymbol);

    BCDocuments::getInstance()->manageSubscriptionInCandlestickData('1' == pIsSubscribed->getValue(), pSymbol->getValue(), sessionID.getTargetCompID().getValue());

    if (prevCnt) { // > 1 threads
        delete pIsSubscribed;
        delete pRelatedSymGroup;
        delete pSymbol;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Deal with incoming orders from clients
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID&) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find userID in NewOrderSingle!" << endl;
        return;
    }

    static FIX::ClOrdID orderID;
    static FIX::Symbol symbol;
    static FIX::OrderQty size;
    static FIX::OrdType orderType;
    static FIX::Price price;

    static FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    static FIX::PartyID userID;

    // #pragma GCC diagnostic ignored ....

    FIX::ClOrdID* pOrderID;
    FIX::Symbol* pSymbol;
    FIX::OrderQty* pSize;
    FIX::OrdType* pOrderType;
    FIX::Price* pPrice;

    FIX50SP2::NewOrderSingle::NoPartyIDs* pIDGroup;
    FIX::PartyID* pUserID;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOrderID = &orderID;
        pSymbol = &symbol;
        pSize = &size;
        pOrderType = &orderType;
        pPrice = &price;
        pIDGroup = &idGroup;
        pUserID = &userID;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pSymbol = new decltype(symbol);
        pSize = new decltype(size);
        pOrderType = new decltype(orderType);
        pPrice = new decltype(price);
        pIDGroup = new decltype(idGroup);
        pUserID = new decltype(userID);
    }

    message.get(*pOrderID);
    message.get(*pSymbol);
    message.get(*pSize);
    message.get(*pOrderType);
    message.get(*pPrice);

    message.getGroup(1, *pIDGroup);
    pIDGroup->get(*pUserID);

    auto usernames = (DBConnector::getInstance()->lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT username FROM traders WHERE id = '" + pUserID->getValue() + "';"));
    synchPrint(COLOR_PROMPT "--------------------------------------\n" NO_COLOR "Order ID: "
        + pOrderID->getValue()
        + "\nUsername: " + (usernames.size() ? usernames[0] : std::string("[???]")) + " (" + pUserID->getValue() + ')'
        + "\n\tType: " + Order::s_typeToString(static_cast<Order::Type>(pOrderType->getValue()))
        + "\n\tSymbol: " + pSymbol->getValue()
        + "\n\tSize: " + std::to_string(pSize->getValue())
        + "\n\tPrice: " + std::to_string(pPrice->getValue()) + '\n');

    // Order validation
    const auto type = static_cast<Order::Type>(pOrderType->getValue());
    int sizeInt = static_cast<int>(pSize->getValue());

    if (pOrderID->getLength() == 0) {
        cout << "orderID is empty" << endl;
        return;
    }

    if (pUserID->getLength() == 0) {
        cout << "userID is empty" << endl;
        return;
    } else if (::STDSTR_NULL == BCDocuments::getInstance()->getTargetIDByUserID(pUserID->getValue())) {
        cout << COLOR_ERROR "This user [" << pUserID->getValue() << "] is not register in the BrokerageCenter" NO_COLOR << endl;
        return;
    }

    if (::isblank(static_cast<int>(type))) {
        cout << "type is empty" << endl;
        return;
    }

    if (pSymbol->getLength() == 0) {
        cout << "symbol is empty" << endl;
        return;
    } else if (!BCDocuments::getInstance()->hasSymbol(pSymbol->getValue())) {
        cout << COLOR_ERROR "This symbol is not register in the Brokerage Center" NO_COLOR << endl;
        return;
    }

    if (sizeInt <= 0) {
        cout << "size is empty" << endl;
        return;
    }

    if (pPrice->getValue() < 0.0) {
        cout << "price is empty" << endl;
        return;
    }

    Order order{
        type,
        pSymbol->getValue(),
        sizeInt,
        pPrice->getValue(),
        pOrderID->getValue(),
        pUserID->getValue()
    };

    BCDocuments::getInstance()->onNewOrderForUserRiskManagement(pUserID->getValue(), std::move(order));

    if (prevCnt) { // > 1 threads
        delete pOrderID;
        delete pSymbol;
        delete pSize;
        delete pOrderType;
        delete pPrice;
        delete pIDGroup;
        delete pUserID;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

#undef SHOW_FROMAPP_MSG
