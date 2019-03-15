#include "FIXInitiator.h"

#include "Stock.h"
#include "globalvariables.h"

#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stocklist;

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
void FIXInitiator::sendExecutionReport(action& report)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
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
    idGroup1.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup1.setField(FIX::PartyID(report.trader_id1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.setField(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup2.setField(FIX::PartyID(report.trader_id2));
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
void FIXInitiator::storeOrder(Quote& order)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getorder_id()));
    message.setField(FIX::Price(order.getprice()));
    message.setField(FIX::Symbol(order.getstockname()));
    message.setField(::FIXFIELD_SIDE_BUY); // Required by FIX
    message.setField(FIX::OrdType(order.getordertype()));
    message.setField(FIX::OrderQty(order.getsize()));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup.set(FIX::PartyID(order.gettrader_id()));
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
         << "Logon - " << sessionID << endl;
}

void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logout - " << sessionID << endl;
}

void FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

// FIXME: Error Message
/**
 * @brief Deal with incoming message of type QuoteAcknowledgement
 */
void FIXInitiator::onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID&) // override
{
    FIX::QuoteID error;
    message.get(error);
    cout << "Error: " << error.getValue() << endl;
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

    FIX::Headline requestID;

    FIX50SP2::News::NoLinesOfText textGroup;
    FIX::Text text;

    message.get(requestID);

    message.getGroup(1, textGroup);
    textGroup.get(text);

    cout << "----- Receive NOTICE -----" << endl;
    cout << "request_id: " << requestID.getValue() << endl;
    cout << "text: " << text.getValue() << endl;
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

    FIX::Symbol symbol;
    FIX::BidPx bidPrice;
    FIX::BidSize bidSize;
    FIX::TransactTime transactTime;

    FIX50SP2::Quote::NoPartyIDs partyGroup1;
    FIX::PartyID buyerID;

    message.get(symbol);
    message.get(bidPrice);
    message.get(bidSize);
    message.get(transactTime);

    message.getGroup(1, partyGroup1);
    partyGroup1.getField(buyerID);

    auto stockIt = stocklist.find(symbol);
    if (stockIt == stocklist.end()) {
        cout << "Receive error in Global!" << endl;
        return;
    }

    Quote newQuote{ symbol, bidPrice, static_cast<int>(bidSize.getValue()) / 100, buyerID, transactTime.getValue() };
    newQuote.setordertype('7'); // Update as "trade" from Global

    long mili = timepara.past_milli(transactTime.getValue());
    newQuote.setmili(mili);

    if (ordType == 0) { // Quote
        FIX::OfferPx askPrice;
        FIX::OfferSize askSize;

        FIX50SP2::Quote::NoPartyIDs partyGroup2;
        FIX::PartyID sellerID;

        message.get(askPrice);
        message.get(askSize);

        message.getGroup(2, partyGroup2);
        partyGroup2.getField(sellerID);

        Quote newQuote2{ symbol, askPrice, static_cast<int>(askSize.getValue()), sellerID, transactTime.getValue() };
        newQuote2.setordertype('8'); // Update as "ask" from Global
        newQuote2.setmili(mili);

        newQuote.setsize(static_cast<int>(bidSize.getValue()));
        newQuote.setordertype('9'); // Update as "bid" from Global

        stockIt->second.buf_new_global(newQuote2);
    }
    stockIt->second.buf_new_global(newQuote);
}
