#include "FIXAcceptor.h"

#include "Stock.h"
#include "globalvariables.h"

#include <atomic>
#include <cassert>
#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stocklist;

/* static */ std::string FIXAcceptor::s_senderID;

// Predefined constant FIX message fields (to avoid recalculations):
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

/*static*/ FIXAcceptor* FIXAcceptor::getInstance()
{
    static FIXAcceptor s_FIXAccInst;
    return &s_FIXAccInst;
}

void FIXAcceptor::addSymbol(const std::string& symbol)
{ // Here no locking is needed only when satisfies precondition: addSymbol was run before others
    m_symbols.insert(symbol);
}

const std::set<std::string>& FIXAcceptor::getSymbols() const
{
    return m_symbols;
}

void FIXAcceptor::connectBrokerageCenter(const std::string& configFile)
{
    disconnectBrokerageCenter();

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

    if (commonDict.has("FileLogPath")) {
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
    } else {
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, true));
    }

    if (commonDict.has("FileStorePath")) {
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
    } else {
        m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    }

    m_acceptorPtr.reset(new FIX::SocketAcceptor(*this, *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Acceptor is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_acceptorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXAcceptor::disconnectBrokerageCenter()
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

/*
 * @brief Send order book update to brokers
 */
void FIXAcceptor::sendOrderBookUpdate2All(Newbook& update)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATEACTION_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(update.getBook()));
    entryGroup.setField(FIX::Symbol(update.getSymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getPrice()));
    entryGroup.setField(FIX::MDEntrySize(update.getSize()));
    auto utc = update.getUtcTime();
    entryGroup.setField(FIX::MDEntryDate(FIX::UtcDateOnly(utc.getDate(), utc.getMonth(), utc.getYear())));
    entryGroup.setField(FIX::MDEntryTime(FIX::UtcTimeOnly(utc.getTimeT(), utc.getFraction(6), 6)));
    entryGroup.setField(FIX::MDMkt(update.getDestination()));

    message.addGroup(entryGroup);

    std::lock_guard<std::mutex> lock(m_mtxTargetList);

    for (const auto& targetID : m_targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/**
 * @brief Sending execution report to brokers
 */
void FIXAcceptor::sendExecutionReport2All(action& report)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID1));
    message.setField(FIX::SecondaryOrderID(report.orderID2));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    message.setField(FIX::OrdStatus(report.decision));
    message.setField(FIX::Symbol(report.stockname));
    message.setField(FIX::Side(report.orderType1));
    message.setField(FIX::OrdType(report.orderType2));
    message.setField(FIX::Price(report.price));
    message.setField(FIX::EffectiveTime(report.exetime, 6));
    message.setField(FIX::LastMkt(report.destination));
    message.setField(::FIXFIELD_LEAVQTY_0); // Required by FIX
    message.setField(FIX::CumQty(report.size));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup1;
    idGroup1.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup1.set(FIX::PartyID(report.traderID1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup2.set(FIX::PartyID(report.traderID2));
    message.addGroup(idGroup2);

    std::lock_guard<std::mutex> lock(m_mtxTargetList);

    for (const auto& targetID : m_targetList) {
        header.setField(FIX::TargetCompID(targetID));
        FIX::Session::sendToTarget(message);
    }
}

/*
 * @brief Send security list to broker
 */
void FIXAcceptor::sendSecurityList(const std::string& targetID)
{
    FIX50SP2::SecurityList message;
    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;

    for (const std::string& symbol : m_symbols) {
        relatedSymGroup.set(FIX::Symbol(symbol));
        message.addGroup(relatedSymGroup);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send order confirmation to broker
 */
void FIXAcceptor::sendOrderConfirmation(const std::string& targetID, const QuoteConfirm& confirmation)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(confirmation.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_ORDER_STATUS); // Required by FIX
    if (confirmation.orderType != '5' && confirmation.orderType != '6') { // Not a cancellation
        message.setField(::FIXFIELD_ORDSTATUS_NEW);
    } else {
        message.setField(::FIXFIELD_ORDSTATUS_PENDING_CANCEL);
    }
    message.setField(FIX::Symbol(confirmation.symbol));
    message.setField(FIX::Side(confirmation.orderType));
    message.setField(FIX::Price(confirmation.price));
    message.setField(FIX::EffectiveTime(confirmation.time, 6));
    message.setField(FIX::LastMkt("SHIFT"));
    message.setField(FIX::LeavesQty(confirmation.size));
    message.setField(::FIXFIELD_CUMQTY_0); // Required by FIX
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup.set(FIX::PartyID(confirmation.clientID));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    std::string targetID = sessionID.getTargetCompID().getValue();
    cout << "Target ID: " << targetID << " logon" << endl;

    // Check whether targetID already exists in target list
    auto pos = m_targetList.find(targetID);
    if (pos == m_targetList.end()) {
        std::lock_guard<std::mutex> lock(m_mtxTargetList);
        m_targetList.insert(targetID);
        cout << "Add new Target ID: " << targetID << endl;
    }

    sendSecurityList(targetID);
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief Deal with the incoming messages which type are NewOrderSingle
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoPartyIDs numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find Client ID in NewOrderSingle!" << endl;
        return;
    }

    static FIX::ClOrdID orderID;
    static FIX::Symbol symbol;
    static FIX::OrderQty size;
    static FIX::OrdType orderType;
    static FIX::Price price;

    static FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    static FIX::PartyID clientID;

    // #pragma GCC diagnostic ignored ....

    FIX::ClOrdID* pOrderID;
    FIX::Symbol* pSymbol;
    FIX::OrderQty* pSize;
    FIX::OrdType* pOrderType;
    FIX::Price* pPrice;

    FIX50SP2::NewOrderSingle::NoPartyIDs* pIDGroup;
    FIX::PartyID* pClientID;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order::memory_order_relaxed);

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
        pClientID = &clientID;
    } else { // > 1 threads; always safe way:
        pOrderID = new decltype(orderID);
        pSymbol = new decltype(symbol);
        pSize = new decltype(size);
        pOrderType = new decltype(orderType);
        pPrice = new decltype(price);
        pIDGroup = new decltype(idGroup);
        pClientID = new decltype(clientID);
    }

    message.get(*pOrderID);
    message.get(*pSymbol);
    message.get(*pSize);
    message.get(*pOrderType);
    message.get(*pPrice);

    message.getGroup(1, *pIDGroup);
    pIDGroup->get(*pClientID);

    long milli = timepara.past_milli();
    FIX::UtcTimeStamp utcNow = timepara.simulationTimestamp();

    Quote quote{ pSymbol->getValue(), pClientID->getValue(), pOrderID->getValue(), pPrice->getValue(), static_cast<int>(pSize->getValue()), pOrderType->getValue(), utcNow };
    quote.setMilli(milli);

    // Add new quote to buffer
    auto stockIt = stocklist.find(pSymbol->getValue());
    if (stockIt != stocklist.end()) {
        stockIt->second.bufNewLocal(quote);
    } else {
        return;
    }

    // Send confirmation to client
    cout << "Sending confirmation: " << pOrderID->getValue() << endl;
    sendOrderConfirmation(sessionID.getTargetCompID().getValue(), { pClientID->getValue(), pOrderID->getValue(), pSymbol->getValue(), pPrice->getValue(), static_cast<int>(pSize->getValue()), pOrderType->getValue(), utcNow });

    if (prevCnt) { // > 1 threads
        delete pOrderID;
        delete pSymbol;
        delete pSize;
        delete pOrderType;
        delete pPrice;
        delete pIDGroup;
        delete pClientID;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}
