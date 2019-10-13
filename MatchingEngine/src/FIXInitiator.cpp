#include "FIXInitiator.h"

#include "StockMarket.h"
#include "TimeSetting.h"

#include <atomic>
#include <cassert>
#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>

/* static */ std::string FIXInitiator::s_senderID;
/* static */ std::string FIXInitiator::s_targetID;

// predefined constant FIX message fields (to avoid recalculations):
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

bool FIXInitiator::connectDatafeedEngine(const std::string& configFile, bool verbose)
{
    disconnectDatafeedEngine();

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

    if (commonDict.has("FileLogPath")) { // store all log events into flat files
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
#ifdef HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLLogDatabase")) { // store all log events into database
        m_logFactoryPtr.reset(new FIX::PostgreSQLLogFactory(settings));
#endif
    } else { // display all log events onto the standard output
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, verbose)); // incoming, outgoing, event
    }

    if (commonDict.has("FileStorePath")) { // store all outgoing messages into flat files
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
#ifdef HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLStoreDatabase")) { // store all outgoing messages into database
        m_messageStoreFactoryPtr.reset(new FIX::PostgreSQLStoreFactory(settings));
#endif
    } else { // store all outgoing messages in memory
        m_messageStoreFactoryPtr.reset(new FIX::MemoryStoreFactory());
    }
    // } else { // do not store messages
    //     m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    // }

    m_initiatorPtr.reset(new FIX::SocketInitiator(*this, *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Initiator is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_initiatorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        return false;
    } catch (const FIX::ConfigError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
        return false;
    }

    return true;
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
 * @brief Send security list and timestamps to Datafeed Engine.
 */
bool FIXInitiator::sendSecurityListRequestAwait(const std::string& requestID, const boost::posix_time::ptime& startTime, const boost::posix_time::ptime& endTime, const std::vector<std::string>& symbols, int numSecondsPerDataChunk)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(requestID));
    message.setField(FIX::SecurityListID(boost::posix_time::to_iso_string(startTime)));
    message.setField(FIX::SecurityListRefID(boost::posix_time::to_iso_string(endTime)));
    message.setField(FIX::SecurityListDesc(std::to_string(numSecondsPerDataChunk)));

    for (size_t i = 0; i < symbols.size(); i++) {
        shift::fix::addFIXGroup<FIX50SP2::SecurityList::NoRelatedSym>(message,
            FIX::Symbol(symbols[i]));
    }

    FIX::Session::sendToTarget(message);

    cout << endl
         << "Please wait for the database signal until TRTH data ready..." << endl;

    std::unique_lock<std::mutex> lock(m_mtxMarketDataReady);
    m_cvMarketDataReady.wait(lock, [this] { return m_lastMarketDataRequestID.length() > 0; });

    const auto pos = m_lastMarketDataRequestID.rfind("[EMPTY]");
    if (pos != std::string::npos) {
        cout << endl
             << COLOR_WARNING "TRTH has no data for request [" << m_lastMarketDataRequestID.substr(0, pos) << "]! Nothing to be sent." NO_COLOR << endl;

        m_lastMarketDataRequestID.clear(); // reset for next use, if necessary, but this might seldomly happen
        return false;
    }
    cout << endl
         << COLOR_PROMPT "TRTH data is ready for request [" << m_lastMarketDataRequestID << "]." NO_COLOR << endl;

    m_lastMarketDataRequestID.clear();
    return true;
}

/**
 * @brief Send market data request to Datafeed Engine.
 */
void FIXInitiator::sendNextDataRequest()
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_SUBSCRIPTIONREQUESTTYPE_SNAPSHOT); // required by FIX
    message.setField(::FIXFIELD_MARKETDEPTH_FULL_BOOK_DEPTH); // required by FIX

    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message, ::FIXFIELD_MDENTRYTYPE_BID);
    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoMDEntryTypes>(message, ::FIXFIELD_MDENTRYTYPE_OFFER);
    shift::fix::addFIXGroup<FIX50SP2::MarketDataRequest::NoRelatedSym>(message, ::FIXFIELD_FAKE_SYMBOL); // empty, not used

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Store order in Database Engine after confirmed.
 */
void FIXInitiator::storeOrder(const Order& order) // FIXME: not used
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getOrderID()));
    message.setField(FIX::Symbol(order.getSymbol()));
    message.setField(FIX::Side(order.getType()));
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::OrdType(order.getType()));
    message.setField(FIX::Price(order.getPrice()));

    shift::fix::addFIXGroup<FIX50SP2::NewOrderSingle::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(order.getTraderID()));

    FIX::Session::sendToTarget(message);
}

void FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
}

void FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_PROMPT "\nLogon:\n[Target] " NO_COLOR << targetID << endl;
}

void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    const auto& targetID = sessionID.getTargetCompID().getValue();
    cout << COLOR_WARNING "\nLogout:\n[Target] " NO_COLOR << targetID << endl;
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
    message.getField(numOfGroups);
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

    message.getField(*pRequestID);

    message.getGroup(1, *pTextGroup);
    pTextGroup->getField(*pText);

    cout << endl;
    cout << "----- Receive NOTICE -----" << endl;
    cout << "request id: " << pRequestID->getValue() << endl;
    cout << "text: " << pText->getValue() << endl;

    bool isReadyNews = pText->getValue() == "READY";
    bool isEmptyNews = !isReadyNews && (pText->getValue() == "EMPTY");
    if (isReadyNews || isEmptyNews) {
        {
            std::lock_guard<std::mutex> guard(m_mtxMarketDataReady);
            m_lastMarketDataRequestID = pRequestID->getValue() + (isEmptyNews ? "[EMPTY]" : "");
        }
        m_cvMarketDataReady.notify_all();
    }

    if (prevCnt) { // > 1 threads
        delete pRequestID;
        delete pTextGroup;
        delete pText;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Receive raw data from Datafeed Engine.
 */
void FIXInitiator::onMessage(const FIX50SP2::Quote& message, const FIX::SessionID&) // override
{
    FIX::QuoteType ordType; // 0 for Quote / 1 for Trade
    message.getField(ordType);

    FIX::NoPartyIDs numOfGroups;
    message.getField(numOfGroups);

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

    message.getField(*pSymbol);
    message.getField(*pBidPrice);
    message.getField(*pBidSize);
    message.getField(*pTransactTime);

    message.getGroup(1, *pIDGroup);
    pIDGroup->getField(*pBuyerID);

    auto stockMarketIt = StockMarketList::getInstance().find(pSymbol->getValue());
    if (stockMarketIt == StockMarketList::getInstance().end()) {
        cout << "Receive error in Global!" << endl;
        return;
    }

    long mili = TimeSetting::getInstance().pastMilli(pTransactTime->getValue());

    Order order{ pSymbol->getValue(), pBidPrice->getValue(), static_cast<int>(pBidSize->getValue()), Order::Type::TRTH_TRADE, pBuyerID->getValue(), pTransactTime->getValue() };
    order.setMilli(mili);

    if (ordType == 0) { // quote
        order.setType(Order::Type::TRTH_BID); // update as "bid" from Global

        message.getField(*pAskPrice);
        message.getField(*pAskSize);

        message.getGroup(2, *pIDGroup);
        pIDGroup->getField(*pSellerID);

        Order order2{ pSymbol->getValue(), pAskPrice->getValue(), static_cast<int>(pAskSize->getValue()), Order::Type::TRTH_ASK, pSellerID->getValue(), pTransactTime->getValue() };
        order2.setMilli(mili);

        stockMarketIt->second.bufNewGlobalOrder(std::move(order2));
    }
    stockMarketIt->second.bufNewGlobalOrder(std::move(order));

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
