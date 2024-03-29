#include "FIXAcceptor.h"

#include "BCDocuments.h"
#include "DBConnector.h"
#include "Parameters.h"

#include <atomic>
#include <cassert>

#include <quickfix/FieldConvertors.h>
#include <quickfix/FieldTypes.h>

#include <shift/miscutils/Common.h>
#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>

/* static */ std::string FIXAcceptor::s_senderID;

// predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_CLEARINGBUSINESSDATE = FIX::ClearingBusinessDate("20181217");
static const auto& FIXFIELD_SYMBOL_CASH = FIX::Symbol("CASH");
static const auto& FIXFIELD_SECURITYTYPE_CASH = FIX::SecurityType(FIX::SecurityType_CASH);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);
static const auto& FIXFIELD_SECURITYTYPE_CS = FIX::SecurityType(FIX::SecurityType_COMMON_STOCK);
static const auto& FIXFIELD_EXECTYPE_ORDER_STATUS = FIX::ExecType(FIX::ExecType_ORDER_STATUS);
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType(FIX::ExecType_TRADE);
static const auto& FIXFIELD_ADVTRANSTYPE_NEW = FIX::AdvTransType(FIX::AdvTransType_NEW);
static const auto& FIXFIELD_ADVSIDE_TRADE = FIX::AdvSide(FIX::AdvSide_TRADE);
static const auto& FIXFIELD_MDUPDATEACTION_CHANGE = FIX::MDUpdateAction(FIX::MDUpdateAction_CHANGE);

FIXAcceptor::~FIXAcceptor() // override
{
    disconnectClients();
}

/* static */ auto FIXAcceptor::getInstance() -> FIXAcceptor&
{
    static FIXAcceptor s_FIXAccInst;
    return s_FIXAccInst;
}

auto FIXAcceptor::connectClients(const std::string& configFile, bool verbose /* = false */, const std::string& cryptoKey /* = "" */, const std::string& dbConfigFile /* = "" */) -> bool
{
    disconnectClients();

    const auto targetIDs = shift::database::readRowsOfField(DBConnector::getInstance().getConn(), "SELECT username FROM traders WHERE super = TRUE;");
    if (targetIDs.empty()) {
        cout << COLOR_WARNING "WARNING: No target is present in the database. The system terminates." NO_COLOR << endl;
        std::terminate();
    }

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

    // Default QuickFIX values for StartTime and EndTime are "00:00:00".
    // This means sessions will reset at midnight UTC time
    // (all sessions will be disconnected and sequence numbers reset),
    // which is not interesting for a simulator that can run at any time.
    // It is also important to set StartTime != EndTime, so that there is
    // a sequence number reset every time a new simulation session starts.
    // We set StartTime to the current time, and EndTime to some hours from
    // now (giving enough time for an usual 6.5-hour trading session).
    auto startTime = FIX::UtcTimeStamp(); // current UTC time
    auto endTime = startTime; // + is not overloaded, so we must use += to add seconds
    endTime += FIX_SESSION_DURATION; // FIX_SESSION_DURATION seconds from now
    std::string startTimeStr = FIX::UtcTimeStampConvertor::convert(startTime); // YYYYMMDD-HH:MM:SS
    std::string endTimeStr = FIX::UtcTimeStampConvertor::convert(endTime);
    commonDict.setString("StartTime", startTimeStr.substr(startTimeStr.size() - 8)); // HH:MM:SS
    commonDict.setString("EndTime", endTimeStr.substr(endTimeStr.size() - 8));
    settings.set(commonDict);

    for (const auto& tarID : targetIDs) {
        FIX::SessionID sid(commonDict.getString("BeginString"), commonDict.getString("SenderCompID") // e.g. BROKERAGECENTER
            ,
            shift::toUpper(tarID) // e.g. WEBCLIENT
        );
        FIX::Dictionary dict;

        settings.set(sid, std::move(dict));
    }

    if (commonDict.has("FileLogPath")) { // store all log events into flat files
        m_logFactoryPtr = std::make_unique<FIX::FileLogFactory>(commonDict.getString("FileLogPath"));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLLogDatabase")) { // store all log events into database
        auto loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, dbConfigFile);
        commonDict.setString("PostgreSQLLogUser", loginInfo["DBUser"]);
        commonDict.setString("PostgreSQLLogPassword", loginInfo["DBPassword"]);
        commonDict.setString("PostgreSQLLogHost", loginInfo["DBHost"]);
        commonDict.setString("PostgreSQLLogPort", loginInfo["DBPort"]);
        settings.set(commonDict);
        m_logFactoryPtr = std::make_unique<FIX::PostgreSQLLogFactory>(settings);
#endif
    } else { // display all log events onto the standard output
        m_logFactoryPtr = std::make_unique<FIX::ScreenLogFactory>(false, false, verbose); // incoming, outgoing, event
    }

    if (commonDict.has("FileStorePath")) { // store all outgoing messages into flat files
        m_messageStoreFactoryPtr = std::make_unique<FIX::FileStoreFactory>(commonDict.getString("FileStorePath"));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLStoreDatabase")) { // store all outgoing messages into database
        auto loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, dbConfigFile);
        commonDict.setString("PostgreSQLStoreUser", loginInfo["DBUser"]);
        commonDict.setString("PostgreSQLStorePassword", loginInfo["DBPassword"]);
        commonDict.setString("PostgreSQLStoreHost", loginInfo["DBHost"]);
        commonDict.setString("PostgreSQLStorePort", loginInfo["DBPort"]);
        settings.set(commonDict);
        m_messageStoreFactoryPtr = std::make_unique<FIX::PostgreSQLStoreFactory>(settings);
#endif
    } else { // store all outgoing messages in memory
        m_messageStoreFactoryPtr = std::make_unique<FIX::MemoryStoreFactory>();
    }
    // } else { // do not store messages
    //     m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    // }

    m_acceptorPtr = std::make_unique<FIX::SocketAcceptor>(*this, *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr);

    cout << '\n'
         << COLOR "Acceptor is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_acceptorPtr->start();
    } catch (const FIX::ConfigError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        return false;
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        return false;
    }

    return true;
}

void FIXAcceptor::disconnectClients()
{
    if (!m_acceptorPtr) {
        return;
    }

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
/* static */ void FIXAcceptor::s_sendLastPrice2All(const Transaction& transac)
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

    for (const auto& targetID : BCDocuments::getInstance().getTargetList()) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

static inline void s_addGroupToOrderBookMsg(FIX::Message& message, const OrderBookEntry& entry)
{
    shift::fix::addFIXGroup<FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries>(message,
        FIX::MDEntryType(entry.getType()),
        FIX::MDEntryPx(entry.getPrice()),
        FIX::MDEntrySize(entry.getSize()),
        FIX::MDEntryDate(entry.getDate()),
        FIX::MDEntryTime(entry.getTime()),
        FIX::MDMkt(entry.getDestination()));
}

/**
 * @brief Send complete order book by type
 */
/* static */ void FIXAcceptor::s_sendOrderBook(const std::vector<std::string>& targetList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBook)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataSnapshotFullRefresh));

    message.setField(FIX::Symbol(orderBook.begin()->second.begin()->second.getSymbol()));

    const auto obt = orderBook.begin()->second.begin()->second.getType();

    if (obt == OrderBookEntry::Type::GLB_BID || obt == OrderBookEntry::Type::LOC_BID) { // reverse the global/local bid order book order
        for (auto ri = orderBook.crbegin(); ri != orderBook.crend(); ++ri) {
            for (const auto& j : ri->second) {
                ::s_addGroupToOrderBookMsg(message, j.second);
            }
        }
    } else { // *_ASK
        for (const auto& i : orderBook) {
            for (const auto& j : i.second) {
                ::s_addGroupToOrderBookMsg(message, j.second);
            }
        }
    }

    for (const auto& targetID : targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/**
 * @brief   Send order book update to one client
 */
/* static */ void FIXAcceptor::s_sendOrderBookUpdate(const std::vector<std::string>& targetList, const OrderBookEntry& update)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    shift::fix::addFIXGroup<FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries>(message,
        ::FIXFIELD_MDUPDATEACTION_CHANGE,
        FIX::MDEntryType(update.getType()),
        FIX::Symbol(update.getSymbol()),
        FIX::MDEntryPx(update.getPrice()),
        FIX::MDEntrySize(update.getSize()),
        FIX::MDEntryDate(update.getDate()),
        FIX::MDEntryTime(update.getTime()),
        FIX::MDMkt(update.getDestination()));

    for (const auto& targetID : targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/* static */ void FIXAcceptor::s_sendCandlestickData(const std::vector<std::string>& targetList, const CandlestickDataPoint& cdPoint)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityStatus));

    message.setField(FIX::Symbol(cdPoint.getSymbol()));
    message.setField(FIX::HighPx(cdPoint.getHighPrice())); // High price
    message.setField(FIX::LowPx(cdPoint.getLowPrice())); // Low price
    message.setField(FIX::LastPx(cdPoint.getClosePrice())); // Close price
    message.setField(FIX::TransactTime(FIX::UtcTimeStamp(cdPoint.getTimeFrom()), 6)); // no need for timestamp
    message.setField(FIX::FirstPx(cdPoint.getOpenPrice())); // Open price

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
/* static */ void FIXAcceptor::s_sendConfirmationReport(const ExecutionReport& report)
{
    const auto& targetID = BCDocuments::getInstance().getTargetIDByUserID(report.userID);
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
    message.setField(FIX::EffectiveTime(report.simulationTime, 6));
    message.setField(FIX::LastMkt(report.destination));
    message.setField(FIX::LeavesQty(report.currentSize));
    message.setField(FIX::CumQty(report.executedSize));
    message.setField(FIX::TransactTime(report.realTime, 6));

    shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(report.userID));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send Portfolio Summary to LC
 */
/* static */ void FIXAcceptor::s_sendPortfolioSummary(const std::string& userID, const PortfolioSummary& summary)
{
    const auto& targetID = BCDocuments::getInstance().getTargetIDByUserID(userID);
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

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(userID));

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPositions>(message,
        FIX::LongQty(summary.getTotalShares()));

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPosAmt>(message,
        FIX::PosAmt(summary.getBuyingPower()));

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPosAmt>(message,
        FIX::PosAmt(summary.getHoldingBalance()));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send Portfolio Item to LC
 */
/* static */ void FIXAcceptor::s_sendPortfolioItem(const std::string& userID, const PortfolioItem& item)
{
    const auto& targetID = BCDocuments::getInstance().getTargetIDByUserID(userID);
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

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(userID));

    shift::fix::addFIXGroup<FIX50SP2::PositionReport::NoPositions>(message,
        FIX::LongQty(item.getLongShares()),
        FIX::ShortQty(item.getShortShares()));

    FIX::Session::sendToTarget(message);
}

/* static */ void FIXAcceptor::s_sendWaitingList(const std::string& userID, const std::unordered_map<std::string, Order>& orders)
{
    const auto& targetID = BCDocuments::getInstance().getTargetIDByUserID(userID);
    if (::STDSTR_NULL == targetID) {
        cout << "sendWaitingList(): ";
        cout << userID << " does not exist: Target computer ID not identified!" << endl;
        return;
    }

    FIX50SP2::NewOrderList message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderList));

    message.setField(FIX::ListID(shift::crossguid::newGuid().str())); // FIX Required
    message.setField(FIX::ClientBidID(userID));
    message.setField(FIX::BidType(1)); // FIX Required
    message.setField(FIX::TotNoOrders(orders.size())); // FIX Required

    int i = 0;
    for (const auto& kv : orders) {
        const auto& order = kv.second;
        shift::fix::addFIXGroup<FIX50SP2::NewOrderList::NoOrders>(message,
            FIX::ClOrdID(order.getID()),
            FIX::ListSeqNo(++i),
            FIX::Symbol(order.getSymbol()),
            FIX::Side(order.getType()),
            FIX::OrdType(order.getType()),
            FIX::OrderQty(order.getSize()),
            FIX::Price(order.getPrice()),
            FIX::OrderQty2(order.getExecutedSize()),
            FIX::PositionEffect(order.getStatus()));
    }

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send the security list
 */
/* static */ void FIXAcceptor::s_sendSecurityList(const std::string& targetID, const std::unordered_set<std::string>& symbols)
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

    for (const auto& symbol : ascSymbols) {
        shift::fix::addFIXGroup<FIX50SP2::SecurityList::NoRelatedSym>(message,
            FIX::Symbol(symbol));
    }

    FIX::Session::sendToTarget(message);
}

/* static */ void FIXAcceptor::s_sendUserIDResponse(const std::string& targetID, const std::string& userReqID, const std::string& username, const std::string& userID)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_UserResponse));

    message.setField(FIX::UserRequestID(userReqID));
    message.setField(FIX::Username(username));
    message.setField(FIX::UserStatus(userID.empty() ? 2 : 1)); // Not Logged In : Logged In
    message.setField(FIX::UserStatusText(userID));

    FIX::Session::sendToTarget(message);
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

    s_sendSecurityList(targetID, BCDocuments::getInstance().getSymbols());
}

/**
 * @brief When super user disconnect from BC
 */
void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;
}

/**
 * @brief Events from the admin/super user
 */
void FIXAcceptor::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) noexcept(false) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();

    if (FIX::MsgType_Logon == message.getHeader().getField(FIX::FIELD::MsgType)) {
        /* Incoming a new connection/logon from super user to BC;
        The complete actions: fromAdmin => onLogon => onMessage(UserRequest)
    */
        std::string adminName; // super user's info to qualify the connection
        std::string adminPsw;

        FIXT11::Logon::NoMsgTypes msgTypeGroup;
        message.getGroup(1, msgTypeGroup);
        adminName = msgTypeGroup.getField(FIX::FIELD::RefMsgType);
        message.getGroup(2, msgTypeGroup);
        adminPsw = msgTypeGroup.getField(FIX::FIELD::RefMsgType);

        const auto pswCol = (DBConnector::getInstance().lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance().getConn(), "SELECT password FROM traders WHERE username = '" + adminName + "';"));
        if (!pswCol.empty() && pswCol.front() == adminPsw) {
            // Grants the connection rights to this super user.
            cout << COLOR_PROMPT "Authentication successful for super user [" << targetID << ':' << adminName << "]." NO_COLOR << endl;
        } else {
            cout << COLOR_ERROR "ERROR: @fromAdmin(): Password of the super user [" << targetID << ':' << adminName << "] is inconsistent with the one recorded in database (in SHA1 format).\nPlease make sure the client-side send matched password to BC reliably." NO_COLOR << endl;

            throw FIX::RejectLogon(); // will disconnect the counterparty
        }
    } else if (FIX::MsgType_Logout == message.getHeader().getField(FIX::FIELD::MsgType)) {
        /* Incoming a new disconnection/logout from super user to BC;
            The complete actions: fromAdmin => onLogout
           If disconnection is caused by connection error, only onLogout is called;
            We want this here - we should not unregister targets based on connection errors
        */
        BCDocuments::getInstance().unregisterTargetFromDoc(targetID); // this shall come first to timely affect other acceptor parts that are sending things aside
        BCDocuments::getInstance().unregisterTargetFromCandles(targetID);
        BCDocuments::getInstance().unregisterTargetFromOrderBooks(targetID);
    }
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) noexcept(false) // override
{
    crack(message, sessionID);
    // synchPrint("\nfromAPP message: " + message + '\n');
}

/**
 * @brief General handler for incoming super & ordinal web users. After this point, usernames are represented by corresponding userIDs.
 */
void FIXAcceptor::onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::Username username;
    message.getField(username);
    FIX::UserRequestID reqID;
    message.getField(reqID);

    auto idCol = (DBConnector::getInstance().lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance().getConn(), "SELECT id FROM traders WHERE username = '" + username.getValue() + "';"));
    if (idCol.empty()) {
        idCol.emplace_back(); // junk userID
        cout << COLOR_ERROR "ERROR: Cannot retrieve id for user [" << username.getValue() << "]. Please check this user that is already added into 'traders'." NO_COLOR << endl;
    }
    const auto& userID = idCol[0];

    s_sendUserIDResponse(sessionID.getTargetCompID().getValue(), reqID, username, userID);
    if (userID.empty()) {
        return;
    }

    BCDocuments::getInstance().registerUserInDoc(sessionID.getTargetCompID().getValue(), userID);
    BCDocuments::getInstance().sendHistoryToUser(userID);

    synchPrint(COLOR_PROMPT "Web user [" NO_COLOR + username.getValue() + COLOR_PROMPT "](" NO_COLOR + userID + COLOR_PROMPT ") was registered.\n\n" NO_COLOR);
}

/*
 * @brief Receive order book subscription request from LC
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find Symbol in MarketDataRequest!" << endl;
        return;
    }

    static FIX::SubscriptionRequestType isSubscribed;

    static FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SubscriptionRequestType* pIsSubscribed = nullptr;

    FIX50SP2::MarketDataRequest::NoRelatedSym* pRelatedSymGroup = nullptr;
    FIX::Symbol* pSymbol = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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

    message.getField(*pIsSubscribed);

    message.getGroup(1, *pRelatedSymGroup);
    pRelatedSymGroup->getField(*pSymbol);

    BCDocuments::getInstance().manageSubscriptionInOrderBook('1' == pIsSubscribed->getValue(), pSymbol->getValue(), sessionID.getTargetCompID().getValue());

    if (0 != prevCnt) { // > 1 threads
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
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) { // make sure there is a symbol in group
        cout << "Cannot find Symbol in RFQRequest" << endl;
        return;
    }

    static FIX::SubscriptionRequestType isSubscribed;

    static FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SubscriptionRequestType* pIsSubscribed = nullptr;

    FIX50SP2::MarketDataRequest::NoRelatedSym* pRelatedSymGroup = nullptr;
    FIX::Symbol* pSymbol = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

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

    message.getField(*pIsSubscribed);

    message.getGroup(1, *pRelatedSymGroup);
    pRelatedSymGroup->getField(*pSymbol);

    BCDocuments::getInstance().manageSubscriptionInCandlestickData('1' == pIsSubscribed->getValue(), pSymbol->getValue(), sessionID.getTargetCompID().getValue());

    if (0 != prevCnt) { // > 1 threads
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
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find userID in NewOrderSingle!" << endl;
        return;
    }

    static FIX::ClOrdID orderID;
    static FIX::Symbol orderSymbol;
    static FIX::OrderQty orderSize;
    static FIX::OrdType orderType;
    static FIX::Price orderPrice;

    static FIX50SP2::NewOrderSingle::NoPartyIDs orderIDGroup;
    static FIX::PartyID orderUserID;

    // #pragma GCC diagnostic ignored ....

    FIX::ClOrdID* pOrderID = nullptr;
    FIX::Symbol* pOrderSymbol = nullptr;
    FIX::OrderQty* pOrderSize = nullptr;
    FIX::OrdType* pOrderType = nullptr;
    FIX::Price* pOrderPrice = nullptr;

    FIX50SP2::NewOrderSingle::NoPartyIDs* pOrderIDGroup = nullptr;
    FIX::PartyID* pOrderUserID = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOrderID = &orderID;
        pOrderSymbol = &orderSymbol;
        pOrderSize = &orderSize;
        pOrderType = &orderType;
        pOrderPrice = &orderPrice;
        pOrderIDGroup = &orderIDGroup;
        pOrderUserID = &orderUserID;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pOrderSymbol = new decltype(orderSymbol);
        pOrderSize = new decltype(orderSize);
        pOrderType = new decltype(orderType);
        pOrderPrice = new decltype(orderPrice);
        pOrderIDGroup = new decltype(orderIDGroup);
        pOrderUserID = new decltype(orderUserID);
    }

    message.getField(*pOrderID);
    message.getField(*pOrderSymbol);
    message.getField(*pOrderSize);
    message.getField(*pOrderType);
    message.getField(*pOrderPrice);

    message.getGroup(1, *pOrderIDGroup);
    pOrderIDGroup->getField(*pOrderUserID);

    std::string id = pOrderID->getValue();
    std::string symbol = pOrderSymbol->getValue();
    int size = static_cast<int>(pOrderSize->getValue());
    auto type = static_cast<Order::Type>(pOrderType->getValue());
    double price = pOrderPrice->getValue();
    std::string userID = pOrderUserID->getValue();

    if (0 != prevCnt) { // > 1 threads
        delete pOrderID;
        delete pOrderSymbol;
        delete pOrderSize;
        delete pOrderType;
        delete pOrderPrice;
        delete pOrderIDGroup;
        delete pOrderUserID;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);

    bool success = true;

    if (id.empty()
        || symbol.empty()
        || (size <= 0)
        || (Order::s_typeToString(type) == "UNKNOWN")
        || (price < 0.0) // market orders have price == 0.0
        || userID.empty()) {
        success = false;
    } else if (!BCDocuments::getInstance().hasSymbol(symbol)) {
        cout << COLOR_ERROR "This symbol is not registered in the Brokerage Center" NO_COLOR << endl;
        success = false;
    } else if (BCDocuments::getInstance().getTargetIDByUserID(userID) == ::STDSTR_NULL) {
        cout << COLOR_ERROR "This user [" << userID << "] is not registered in the BrokerageCenter" NO_COLOR << endl;
        success = false;
    }

    auto usernames = (DBConnector::getInstance().lockPSQL(), shift::database::readRowsOfField(DBConnector::getInstance().getConn(), "SELECT username FROM traders WHERE id = '" + userID + "';"));
    synchPrint(COLOR_PROMPT "--------------------------------------\n\n" NO_COLOR "Order ID: "
        + id
        + "\nUsername: " + (!usernames.empty() ? usernames[0] : std::string("[???]")) + " (" + userID + ')'
        + "\n\tType: " + Order::s_typeToString(type)
        + "\n\tSymbol: " + symbol
        + "\n\tSize: " + std::to_string(size)
        + "\n\tPrice: " + std::to_string(price) + '\n'
        + "\n\tSuccess: " + (success ? "True" : "False") + '\n');

    if (success) {
        Order order { type, symbol, size, price, id, userID };
        BCDocuments::getInstance().onNewOrderForUserRiskManagement(userID, std::move(order));
    } else {
        ExecutionReport report {
            userID,
            id,
            type,
            symbol,
            size,
            0, // executed size
            price,
            Order::Status::REJECTED,
            "BC" // destination (rejected at the Brokerage Center)
        };
        FIXAcceptor::s_sendConfirmationReport(report);
    }
}
