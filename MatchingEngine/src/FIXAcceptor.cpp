#include "FIXAcceptor.h"

#include "Stock.h"
#include "globalvariables.h"

#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stocklist;

/* static */ std::string FIXAcceptor::s_senderID;

// Predefined constant FIX fields (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1"); // FIX BeginString
static const auto& FIXFIELD_MDUPDATE_CHANGE = FIX::MDUpdateAction('1');
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType(FIX::ExecType_TRADE);
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0); // Quantity open for further execution
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = Client ID in FIX4.2
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

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATE_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(update.getbook()));
    entryGroup.setField(FIX::Symbol(update.getsymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getprice()));
    entryGroup.setField(FIX::MDEntrySize(update.getsize()));
    auto utc = update.getutctime();
    entryGroup.setField(FIX::MDEntryDate(FIX::UtcDateOnly(utc.getDate(), utc.getMonth(), utc.getYear())));
    entryGroup.setField(FIX::MDEntryTime(FIX::UtcTimeOnly(utc.getTimeT(), utc.getFraction(6), 6)));
    entryGroup.setField(FIX::MDMkt(update.getdestination()));

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
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.order_id1));
    message.setField(FIX::SecondaryOrderID(report.order_id2));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    message.setField(FIX::OrdStatus(report.decision));
    message.setField(FIX::Symbol(report.stockname));
    message.setField(FIX::Side(report.order_type1));
    message.setField(FIX::OrdType(report.order_type2));
    message.setField(FIX::Price(report.price));
    message.setField(FIX::EffectiveTime(report.exetime, 6));
    message.setField(FIX::LastMkt(report.destination));
    message.setField(::FIXFIELD_LEAVQTY_0); // Required by FIX
    message.setField(FIX::CumQty(report.size));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup1;
    idGroup1.set(::FIXFIELD_CLIENTID);
    idGroup1.set(FIX::PartyID(report.trader_id1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.set(::FIXFIELD_CLIENTID);
    idGroup2.set(FIX::PartyID(report.trader_id2));
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
    header.setField(::FIXFIELD_BEGSTR);
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
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(confirmation.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_ORDER_STATUS); // Required by FIX
    if (confirmation.ordertype != '5' && confirmation.ordertype != '6') { // Not a cancellation
        message.setField(::FIXFIELD_ORDSTATUS_NEW);
    } else {
        message.setField(::FIXFIELD_ORDSTATUS_PENDING_CANCEL);
    }
    message.setField(FIX::Symbol(confirmation.symbol));
    message.setField(FIX::Side(confirmation.ordertype));
    message.setField(FIX::Price(confirmation.price));
    message.setField(FIX::EffectiveTime(confirmation.time, 6));
    message.setField(FIX::LastMkt("SHIFT"));
    message.setField(FIX::LeavesQty(confirmation.size));
    message.setField(::FIXFIELD_CUMQTY_0); // Required by FIX
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_CLIENTID);
    idGroup.set(FIX::PartyID(confirmation.clientID));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    std::string targetID = sessionID.getTargetCompID();
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
    FIX::NoPartyIDs numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find Client ID in NewOrderSingle!" << endl;
        return;
    }

    FIX::ClOrdID orderID;
    FIX::Symbol symbol;
    FIX::OrderQty orderQty;
    FIX::OrdType ordType;
    FIX::Price price;

    message.get(orderID);
    message.get(symbol);
    message.get(orderQty);
    message.get(ordType);
    message.get(price);

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    FIX::PartyID clientID;
    message.getGroup(1, idGroup);
    idGroup.get(clientID);

    long milli = timepara.past_milli();
    FIX::UtcTimeStamp utc_now = timepara.simulationTimestamp();

    Quote quote(symbol, clientID, orderID, price, orderQty, ordType, utc_now);
    quote.setmili(milli);

    // Input the newquote into buffer
    std::map<std::string, Stock>::iterator stockIt;
    stockIt = stocklist.find(symbol);
    if (stockIt != stocklist.end()) {
        stockIt->second.buf_new_local(quote);
    } else {
        return;
    }

    // Send confirmation to client
    cout << "Sending confirmation: " << orderID << endl;
    sendOrderConfirmation(sessionID.getTargetCompID(), { clientID, orderID, symbol, price, static_cast<int>(orderQty), ordType, utc_now });
}
