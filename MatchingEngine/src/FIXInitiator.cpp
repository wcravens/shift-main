#include "FIXInitiator.h"

#include "Stock.h"
#include "TimeSetting.h"

#include <atomic>
#include <cassert>
#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stockList;

/* static */ std::string FIXInitiator::s_senderID;
/* static */ std::string FIXInitiator::s_targetID;

// Predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SNAPSHOT = FIX::SubscriptionRequestType(FIX::SubscriptionRequestType_SNAPSHOT);
static const auto& FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH = FIX::MarketDepth(0);
static const auto& FIXFIELD_MDENTRYTYPE_BID = FIX::MDEntryType(FIX::MDEntryType_BID);
static const auto& FIXFIELD_MDENTRYTYPE_OFFER = FIX::MDEntryType(FIX::MDEntryType_OFFER);
static const auto& FIXFIELD_FAKE_SYMBOL = FIX::Symbol("0");
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType(FIX::ExecType_TRADE);
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);
static const auto& FIXFIELD_SIDE_BUY = FIX::Side(FIX::Side_BUY);

FIXInitiator::~FIXInitiator() // override
{
    disconnectDatafeedEngine();
}

/*static*/ FIXInitiator* FIXInitiator::getInstance()
{
    static FIXInitiator s_FIXInitInst;
    return &s_FIXInitInst;
}

void FIXInitiator::connectDatafeedEngine(const std::string& configFile)
{
    disconnectDatafeedEngine();

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

    m_initiatorPtr.reset(new FIX::SocketInitiator(*this, *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Initiator is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_initiatorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXInitiator::disconnectDatafeedEngine()
{
    if (!m_initiatorPtr)
        return;

    cout << '\n'
         << COLOR "Initiator is stopping..." NO_COLOR << '\n'
         << flush;

    m_initiatorPtr->stop();
    m_initiatorPtr = nullptr;
    m_messageStoreFactoryPtr = nullptr;
    m_logFactoryPtr = nullptr;
}

/**
 * @brief Send security list and timestamps to Datafeed Engine
 */
void FIXInitiator::sendSecurityList(const std::string& requestID, const boost::posix_time::ptime& startTime, const boost::posix_time::ptime& endTime, const std::vector<std::string>& symbols)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(requestID));
    message.setField(FIX::SecurityListID(to_iso_string(startTime)));
    message.setField(FIX::SecurityListRefID(to_iso_string(endTime)));

    for (size_t i = 0; i < symbols.size(); i++) {
        FIX50SP2::SecurityList::NoRelatedSym group;
        group.set(FIX::Symbol(symbols[i]));
        message.addGroup(group);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send market data request to Datafeed Engine
 */
void FIXInitiator::sendMarketDataRequest()
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SNAPSHOT); // Required by FIX
    message.setField(::FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH); // Required by FIX

    FIX50SP2::MarketDataRequest::NoMDEntryTypes typeGroup1;
    FIX50SP2::MarketDataRequest::NoMDEntryTypes typeGroup2;
    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup; // Empty, not used
    typeGroup1.set(::FIXFIELD_MDENTRYTYPE_BID);
    typeGroup2.set(::FIXFIELD_MDENTRYTYPE_OFFER);
    relatedSymGroup.set(::FIXFIELD_FAKE_SYMBOL);

    message.addGroup(typeGroup1);
    message.addGroup(typeGroup2);
    message.addGroup(relatedSymGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Send execution report to Datafeed Engine
 */
void FIXInitiator::sendExecutionReport(const ExecutionReport& report)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(report.orderID1));
    message.setField(FIX::SecondaryOrderID(report.orderID2));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    message.setField(FIX::OrdStatus(report.decision));
    message.setField(FIX::Symbol(report.symbol));
    message.setField(FIX::Side(report.orderType1));
    message.setField(FIX::OrdType(report.orderType2));
    message.setField(FIX::Price(report.price));
    message.setField(FIX::EffectiveTime(report.execTime, 6));
    message.setField(FIX::LastMkt(report.destination));
    message.setField(::FIXFIELD_LEAVQTY_0); // Required by FIX
    message.setField(FIX::CumQty(report.size));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup1;
    idGroup1.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup1.setField(FIX::PartyID(report.traderID1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup2.setField(FIX::PartyID(report.traderID2));
    message.addGroup(idGroup2);

    FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup1;
    timeGroup1.setField(FIX::TrdRegTimestamp(report.time1, 6));
    message.addGroup(timeGroup1);

    FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup2;
    timeGroup2.setField(FIX::TrdRegTimestamp(report.time2, 6));
    message.addGroup(timeGroup2);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Store order in Database Engine after confirmed
 */
void FIXInitiator::storeOrder(const Quote& order)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getOrderID()));
    message.setField(FIX::Price(order.getPrice()));
    message.setField(FIX::Symbol(order.getSymbol()));
    message.setField(::FIXFIELD_SIDE_BUY); // Required by FIX
    message.setField(FIX::OrdType(order.getOrderType()));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup.set(FIX::PartyID(order.getTraderID()));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

void FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
}

void FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logon - " << sessionID.toString() << endl;
}

void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logout - " << sessionID.toString() << endl;
}

void FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief Receive notification that the requested work has been done.
 */
void FIXInitiator::onMessage(const FIX50SP2::News& message, const FIX::SessionID&) // override
{
    FIX::NoLinesOfText numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find Text in Notice!" << endl;
        return;
    }

    static FIX::Headline requestID;

    static FIX50SP2::News::NoLinesOfText textGroup;
    static FIX::Text text;

    // #pragma GCC diagnostic ignored ....

    FIX::Headline* pRequestID;

    FIX50SP2::News::NoLinesOfText* pTextGroup;
    FIX::Text* pText;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pRequestID = &requestID;
        pTextGroup = &textGroup;
        pText = &text;
    } else { // > 1 threads; always safe way:
        pRequestID = new decltype(requestID);
        pTextGroup = new decltype(textGroup);
        pText = new decltype(text);
    }

    message.get(*pRequestID);

    message.getGroup(1, *pTextGroup);
    pTextGroup->get(*pText);

    cout << "----- Receive NOTICE -----" << endl;
    cout << "request_id: " << pRequestID->getValue() << endl;
    cout << "text: " << pText->getValue() << endl;

    if (prevCnt) { // > 1 threads
        delete pRequestID;
        delete pTextGroup;
        delete pText;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Receive raw data from Datafeed Engine
 */
void FIXInitiator::onMessage(const FIX50SP2::Quote& message, const FIX::SessionID&) // override
{
    FIX::QuoteType ordType; // 0 for Quote / 1 for Trade
    message.get(ordType);

    FIX::NoPartyIDs numOfGroups;
    message.get(numOfGroups);

    if ((ordType.getValue() == 0 && numOfGroups.getValue() < 2) || (numOfGroups.getValue() < 1)) {
        cout << "Cannot find enough Party IDs!" << endl;
        return;
    }

    static FIX::Symbol symbol;
    static FIX::BidPx bidPrice;
    static FIX::OfferPx askPrice;
    static FIX::BidSize bidSize;
    static FIX::OfferSize askSize;
    static FIX::TransactTime transactTime;

    static FIX50SP2::Quote::NoPartyIDs idGroup;
    static FIX::PartyID buyerID;
    static FIX::PartyID sellerID;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pSymbol;
    FIX::BidPx* pBidPrice;
    FIX::OfferPx* pAskPrice;
    FIX::BidSize* pBidSize;
    FIX::OfferSize* pAskSize;
    FIX::TransactTime* pTransactTime;

    FIX50SP2::Quote::NoPartyIDs* pIDGroup;
    FIX::PartyID* pBuyerID;
    FIX::PartyID* pSellerID;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pSymbol = &symbol;
        pBidPrice = &bidPrice;
        pAskPrice = &askPrice;
        pBidSize = &bidSize;
        pAskSize = &askSize;
        pTransactTime = &transactTime;
        pIDGroup = &idGroup;
        pBuyerID = &buyerID;
        pSellerID = &sellerID;
    } else { // > 1 threads; always safe way:
        pSymbol = new decltype(symbol);
        pBidPrice = new decltype(bidPrice);
        pAskPrice = new decltype(askPrice);
        pBidSize = new decltype(bidSize);
        pAskSize = new decltype(askSize);
        pTransactTime = new decltype(transactTime);
        pIDGroup = new decltype(idGroup);
        pBuyerID = new decltype(buyerID);
        pSellerID = new decltype(sellerID);
    }

    message.get(*pSymbol);
    message.get(*pBidPrice);
    message.get(*pBidSize);
    message.get(*pTransactTime);

    message.getGroup(1, *pIDGroup);
    pIDGroup->getField(*pBuyerID);

    auto stockIt = stockList.find(pSymbol->getValue());
    if (stockIt == stockList.end()) {
        cout << "Receive error in Global!" << endl;
        return;
    }

    Quote newQuote{ pSymbol->getValue(), pBidPrice->getValue(), static_cast<int>(pBidSize->getValue()) / 100, pBuyerID->getValue(), pTransactTime->getValue() };
    newQuote.setOrderType('7'); // Update as "trade" from Global

    long mili = globalTimeSetting.pastMilli(pTransactTime->getValue());
    newQuote.setMilli(mili);

    if (ordType == 0) { // Quote

        message.get(*pAskPrice);
        message.get(*pAskSize);

        message.getGroup(2, *pIDGroup);
        pIDGroup->getField(*pSellerID);

        Quote newQuote2{ pSymbol->getValue(), pAskPrice->getValue(), static_cast<int>(pAskSize->getValue()), pSellerID->getValue(), pTransactTime->getValue() };
        newQuote2.setOrderType('8'); // Update as "ask" from Global
        newQuote2.setMilli(mili);

        newQuote.setSize(static_cast<int>(pBidSize->getValue()));
        newQuote.setOrderType('9'); // Update as "bid" from Global

        stockIt->second.bufNewGlobal(newQuote2);
    }
    stockIt->second.bufNewGlobal(newQuote);

    if (prevCnt) { // > 1 threads
        delete pSymbol;
        delete pBidPrice;
        delete pAskPrice;
        delete pBidSize;
        delete pAskSize;
        delete pTransactTime;
        delete pIDGroup;
        delete pBuyerID;
        delete pSellerID;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}
