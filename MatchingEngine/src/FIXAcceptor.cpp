#include "FIXAcceptor.h"

#include "Order.h"
#include "Parameters.h"
#include "TimeSetting.h"
#include "markets/StockMarket.h"

#include <atomic>
#include <cassert>
#include <map>

#include <quickfix/FieldConvertors.h>
#include <quickfix/FieldTypes.h>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>

/* static */ std::string FIXAcceptor::s_senderID;

// predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_MDUPDATEACTION_CHANGE = FIX::MDUpdateAction(FIX::MDUpdateAction_CHANGE);
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType(FIX::ExecType_TRADE);
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);
static const auto& FIXFIELD_EXECTYPE_ORDER_STATUS = FIX::ExecType(FIX::ExecType_ORDER_STATUS);
static const auto& FIXFIELD_ORDSTATUS_NEW = FIX::OrdStatus(FIX::OrdStatus_NEW);
static const auto& FIXFIELD_ORDSTATUS_PENDING_CANCEL = FIX::OrdStatus(FIX::OrdStatus_PENDING_CANCEL);
static const auto& FIXFIELD_CUMQTY_0 = FIX::CumQty(0);

FIXAcceptor::~FIXAcceptor() // override
{
    disconnectBrokerageCenter();
}

/* static */ auto FIXAcceptor::getInstance() -> FIXAcceptor&
{
    static FIXAcceptor s_FIXAccInst;
    return s_FIXAccInst;
}

auto FIXAcceptor::connectBrokerageCenter(const std::string& configFile, bool verbose /* = false */, const std::string& cryptoKey /* = "" */, const std::string& dbConfigFile /* = "" */) -> bool
{
    disconnectBrokerageCenter();

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

void FIXAcceptor::disconnectBrokerageCenter()
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

/*
 * @brief Send complete order book to brokers.
 */
void FIXAcceptor::sendOrderBook(const std::vector<OrderBookEntry>& orderBook, const std::string& targetID /* = "" */)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataSnapshotFullRefresh));

    message.setField(FIX::Symbol(orderBook.begin()->getSymbol()));

    for (const auto& entry : orderBook) {
        auto utc = entry.getUTCTime();

        shift::fix::addFIXGroup<FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries>(message,
            FIX::MDEntryType(entry.getType()),
            FIX::MDEntryPx(entry.getPrice()),
            FIX::MDEntrySize(entry.getSize()),
            FIX::MDEntryDate(FIX::UtcDateOnly(utc.getDate(), utc.getMonth(), utc.getYear())),
            FIX::MDEntryTime(FIX::UtcTimeOnly(utc.getTimeT(), utc.getFraction(6), 6)),
            FIX::MDMkt(entry.getDestination()));
    }

    if (targetID.empty()) {
        std::lock_guard<std::mutex> lock(m_mtxTargetSet);

        for (const auto& tID : m_targetSet) {
            header.setField(FIX::TargetCompID(tID));
            FIX::Session::sendToTarget(message);
        }
    } else {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/*
 * @brief Send all order book updates to brokers.
 */
void FIXAcceptor::sendOrderBookUpdates(const std::vector<OrderBookEntry>& orderBookUpdates)
{
    for (const auto& update : orderBookUpdates) {
        FIX::Message message;

        FIX::Header& header = message.getHeader();
        header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
        header.setField(FIX::SenderCompID(s_senderID));
        header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

        auto utc = update.getUTCTime();

        shift::fix::addFIXGroup<FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries>(message,
            ::FIXFIELD_MDUPDATEACTION_CHANGE,
            FIX::MDEntryType(update.getType()),
            FIX::Symbol(update.getSymbol()),
            FIX::MDEntryPx(update.getPrice()),
            FIX::MDEntrySize(update.getSize()),
            FIX::MDEntryDate(FIX::UtcDateOnly(utc.getDate(), utc.getMonth(), utc.getYear())),
            FIX::MDEntryTime(FIX::UtcTimeOnly(utc.getTimeT(), utc.getFraction(6), 6)),
            FIX::MDMkt(update.getDestination()));

        {
            std::lock_guard<std::mutex> lock(m_mtxTargetSet);

            for (const auto& tID : m_targetSet) {
                header.setField(FIX::TargetCompID(tID));
                FIX::Session::sendToTarget(message);
            }
        }
    }
}

/**
 * @brief Sending all execution reports to brokers.
 */
void FIXAcceptor::sendExecutionReports(const std::vector<ExecutionReport>& executionReports)
{
    for (const auto& report : executionReports) {
        FIX::Message message;

        FIX::Header& header = message.getHeader();
        header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
        header.setField(FIX::SenderCompID(s_senderID));
        header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

        message.setField(FIX::OrderID(report.orderID1));
        message.setField(FIX::SecondaryOrderID(report.orderID2));
        message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
        message.setField(::FIXFIELD_EXECTYPE_TRADE); // required by FIX
        message.setField(FIX::OrdStatus(report.decision));
        message.setField(FIX::Symbol(report.symbol));
        message.setField(FIX::Side(report.orderType1));
        message.setField(FIX::OrdType(report.orderType2));
        message.setField(FIX::Price(report.price));
        message.setField(FIX::EffectiveTime(TimeSetting::getInstance().simulationTimestamp(), 6));
        message.setField(FIX::LastMkt(report.destination));
        message.setField(::FIXFIELD_LEAVQTY_0); // required by FIX
        message.setField(FIX::CumQty(report.size));
        message.setField(FIX::TransactTime(6));

        shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoPartyIDs>(message,
            ::FIXFIELD_PARTYROLE_CLIENTID,
            FIX::PartyID(report.traderID1));

        shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoPartyIDs>(message,
            ::FIXFIELD_PARTYROLE_CLIENTID,
            FIX::PartyID(report.traderID2));

        shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoTrdRegTimestamps>(message,
            FIX::TrdRegTimestamp(report.simulationTime1, 6));

        shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoTrdRegTimestamps>(message,
            FIX::TrdRegTimestamp(report.simulationTime2, 6));

        {
            std::lock_guard<std::mutex> lock(m_mtxTargetSet);

            for (const auto& tID : m_targetSet) {
                header.setField(FIX::TargetCompID(tID));
                FIX::Session::sendToTarget(message);
            }
        }
    }
}

/*
 * @brief Send security list to broker.
 */
/* static */ void FIXAcceptor::s_sendSecurityList(const std::string& targetID)
{
    FIX50SP2::SecurityList message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    for (const auto& kv : markets::StockMarketList::getInstance()) {
        shift::fix::addFIXGroup<FIX50SP2::SecurityList::NoRelatedSym>(message,
            FIX::Symbol(kv.first));
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send order confirmation to broker.
 */
/* static */ void FIXAcceptor::s_sendOrderConfirmation(const OrderConfirmation& confirmation, const std::string& targetID)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(confirmation.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_ORDER_STATUS); // required by FIX
    if (confirmation.orderType != Order::Type::CANCEL_BID && confirmation.orderType != Order::Type::CANCEL_ASK) { // not a cancellation
        message.setField(::FIXFIELD_ORDSTATUS_NEW);
    } else {
        message.setField(::FIXFIELD_ORDSTATUS_PENDING_CANCEL);
    }
    message.setField(FIX::Symbol(confirmation.symbol));
    message.setField(FIX::Side(confirmation.orderType));
    message.setField(FIX::Price(confirmation.price));
    message.setField(FIX::EffectiveTime(TimeSetting::getInstance().simulationTimestamp(), 6));
    message.setField(FIX::LastMkt("SHIFT"));
    message.setField(FIX::LeavesQty(confirmation.size));
    message.setField(::FIXFIELD_CUMQTY_0); // required by FIX
    message.setField(FIX::TransactTime(6));

    shift::fix::addFIXGroup<FIX50SP2::ExecutionReport::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(confirmation.traderID));

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_PROMPT "\nLogon:\n[Target] " NO_COLOR << targetID << endl;

    s_sendSecurityList(targetID);

    // send current order book data of all securities to connecting target
    for (auto& kv : markets::StockMarketList::getInstance()) {
        kv.second->sendOrderBookData(true, targetID);
    }

    {
        std::lock_guard<std::mutex> lock(m_mtxTargetSet);
        m_targetSet.insert(targetID);
    }
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;

    // if we do not remove this targetID from the target set,
    // messages sent during a disconnection will be resent upon reconnection

    // {
    //     std::lock_guard<std::mutex> lock(m_mtxTargetSet);
    //     m_targetList.erase(m_targetSet);
    // }
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) noexcept(false) // override
{
    crack(message, sessionID);
}

/**
 * @brief Deal with the incoming messages which type are NewOrderSingle.
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find Trader ID in NewOrderSingle!" << endl;
        return;
    }

    static FIX::ClOrdID orderID;
    static FIX::Symbol symbol;
    static FIX::OrderQty size;
    static FIX::OrdType orderType;
    static FIX::Price price;

    static FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    static FIX::PartyID traderID;

    // #pragma GCC diagnostic ignored ....

    FIX::ClOrdID* pOrderID = nullptr;
    FIX::Symbol* pSymbol = nullptr;
    FIX::OrderQty* pSize = nullptr;
    FIX::OrdType* pOrderType = nullptr;
    FIX::Price* pPrice = nullptr;

    FIX50SP2::NewOrderSingle::NoPartyIDs* pIDGroup = nullptr;
    FIX::PartyID* pTraderID = nullptr;

    static std::atomic<unsigned int> s_cntAtom { 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1)) {
    }

    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pOrderID = &orderID;
        pSymbol = &symbol;
        pSize = &size;
        pOrderType = &orderType;
        pPrice = &price;
        pIDGroup = &idGroup;
        pTraderID = &traderID;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pSymbol = new decltype(symbol);
        pSize = new decltype(size);
        pOrderType = new decltype(orderType);
        pPrice = new decltype(price);
        pIDGroup = new decltype(idGroup);
        pTraderID = new decltype(traderID);
    }

    message.getField(*pOrderID);
    message.getField(*pSymbol);
    message.getField(*pSize);
    message.getField(*pOrderType);
    message.getField(*pPrice);

    message.getGroup(1, *pIDGroup);
    pIDGroup->getField(*pTraderID);

    long milli = TimeSetting::getInstance().pastMilli();
    FIX::UtcTimeStamp now = TimeSetting::getInstance().simulationTimestamp();

    Order order { pSymbol->getValue(), pTraderID->getValue(), pOrderID->getValue(), pPrice->getValue(), static_cast<int>(pSize->getValue()), static_cast<Order::Type>(pOrderType->getValue()), now };
    order.setMilli(milli);

    // add new quote to buffer
    auto stockMarketIt = markets::StockMarketList::getInstance().find(pSymbol->getValue());
    if (stockMarketIt != markets::StockMarketList::getInstance().end()) {
        stockMarketIt->second->bufNewLocalOrder(std::move(order));
    } else {
        return;
    }

    // send confirmation to client
    cout << "Sending confirmation: " << pOrderID->getValue() << endl;
    s_sendOrderConfirmation({ pSymbol->getValue(), pTraderID->getValue(), pOrderID->getValue(), pPrice->getValue(), static_cast<int>(pSize->getValue()), pOrderType->getValue() }, sessionID.getTargetCompID().getValue());

    if (0 != prevCnt) { // > 1 threads
        delete pOrderID;
        delete pSymbol;
        delete pSize;
        delete pOrderType;
        delete pPrice;
        delete pIDGroup;
        delete pTraderID;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}
