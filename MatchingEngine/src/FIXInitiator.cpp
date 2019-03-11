#include "FIXInitiator.h"

#include "Stock.h"
#include "globalvariables.h"

#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stocklist;

/* static */ std::string FIXInitiator::s_senderID;
/* static */ std::string FIXInitiator::s_targetID;

// Predefined constant FIX fields (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1"); // FIX BeginString
static const auto& FIXFIELD_SUBTYPE_SNAP = FIX::SubscriptionRequestType('0'); // 0 = Snapshot
static const auto& FIXFIELD_DOM_FULL = FIX::MarketDepth(0); // Depth of market for Book Snapshot, 0 = full book depth
static const auto& FIXFIELD_ENTRY_BID = FIX::MDEntryType('0'); // Type of market data entry, 0 = Bid
static const auto& FIXFIELD_ENTRY_OFFER = FIX::MDEntryType('1'); // Type of market data entry, 1 = Offer
static const auto& FIXFIELD_SYMBOL = FIX::Symbol("1"); // Ticker symbol
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType('F'); // F = Trade
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0); // Quantity open for further execution
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = ClientID in FIX4.2
static const auto& FIXFIELD_EXECBROKER = FIX::PartyRole(1); // 1 = ExecBroker in FIX4.2
static const auto& FIXFIELD_SIDE_BUY = FIX::Side('1'); // 1 = Buy

FIXInitiator::~FIXInitiator() // override
{
    disconnectDatafeedEngine();
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
 * @brief Send security list and timestamp to DE 
 */
void FIXInitiator::sendSecurityList(const std::string& request_id, const boost::posix_time::ptime& start_time, const boost::posix_time::ptime& end_time, const std::vector<std::string>& symbols)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(request_id));
    message.setField(FIX::SecurityListID(to_iso_string(start_time)));
    message.setField(FIX::SecurityListRefID(to_iso_string(end_time)));

    for (size_t i = 0; i < symbols.size(); i++) {
        FIX50SP2::SecurityList::NoRelatedSym group;
        group.set(FIX::Symbol(symbols[i]));
        message.addGroup(group);
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief send Market Data Request to DE
 */
void FIXInitiator::sendMarketDataRequest()
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataRequest));

    message.setField(FIX::MDReqID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_SUBTYPE_SNAP); // Required by FIX
    message.setField(::FIXFIELD_DOM_FULL); // Required by FIX

    FIX50SP2::MarketDataRequest::NoMDEntryTypes typeGroup1;
    FIX50SP2::MarketDataRequest::NoMDEntryTypes typeGroup2;
    FIX50SP2::MarketDataRequest::NoRelatedSym relatedSymGroup; // Empty, not used
    typeGroup1.set(::FIXFIELD_ENTRY_BID);
    typeGroup2.set(::FIXFIELD_ENTRY_OFFER);
    relatedSymGroup.set(::FIXFIELD_SYMBOL);

    message.addGroup(typeGroup1);
    message.addGroup(typeGroup2);
    message.addGroup(relatedSymGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Storing the action records into database
 */
/* static */ void FIXInitiator::SendActionRecord(action& actions)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(actions.order_id1));
    message.setField(FIX::SecondaryOrderID(actions.order_id2));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    message.setField(FIX::OrdStatus(actions.decision));
    message.setField(FIX::Symbol(actions.stockname));
    message.setField(FIX::Side(actions.order_type1));
    message.setField(FIX::OrdType(actions.order_type2));
    message.setField(FIX::Price(actions.price));
    message.setField(FIX::EffectiveTime(actions.exetime, 6));
    message.setField(FIX::LastMkt(actions.destination));
    message.setField(::FIXFIELD_LEAVQTY_0); // Required by FIX
    message.setField(FIX::CumQty(actions.size));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup1;
    idGroup1.setField(::FIXFIELD_CLIENTID);
    idGroup1.setField(FIX::PartyID(actions.trader_id1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.setField(::FIXFIELD_CLIENTID);
    idGroup2.setField(FIX::PartyID(actions.trader_id2));
    message.addGroup(idGroup2);

    FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup1;
    timeGroup1.setField(FIX::TrdRegTimestamp(actions.time1, 6));
    message.addGroup(timeGroup1);

    FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup2;
    timeGroup2.setField(FIX::TrdRegTimestamp(actions.time2, 6));
    message.addGroup(timeGroup2);

    FIX::Session::sendToTarget(message);
}

// Quote(string stockname1, string trader_id1, string order_id1, double price1, int size1, string time1, char ordertype1);
/**
 * @brief Store orders to the Database after confirmed
 */
/* static */ void FIXInitiator::StoreOrder(Quote& quote)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(quote.getorder_id()));
    message.setField(FIX::Price(quote.getprice()));
    message.setField(FIX::Symbol(quote.getstockname()));
    message.setField(::FIXFIELD_SIDE_BUY); // Required by FIX
    message.setField(FIX::OrdType(quote.getordertype()));
    message.setField(FIX::OrderQty(quote.getsize()));
    message.setField(FIX::TransactTime(6));

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_CLIENTID);
    idGroup.set(FIX::PartyID(quote.gettrader_id()));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

void FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID();
    s_targetID = sessionID.getTargetCompID();
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
 * @brief Deal with the incoming message which type is QuoteAcknowledgement
 */
void FIXInitiator::onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID&) // override
{
    FIX::QuoteID error;
    message.get(error);
    cout << "Error: " << error << endl;
}

/**
 * @brief Receive message to notify that 
 * the requested works have been done.
 */
void FIXInitiator::onMessage(const FIX50SP2::News& message, const FIX::SessionID&) // override
{
    FIX::NoLinesOfText numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find Text in Notice\n";
        return;
    }

    FIX50SP2::News::NoLinesOfText textGroup;
    message.getGroup(1, textGroup);
    FIX::Text text;
    textGroup.get(text);

    FIX::Headline request_id;
    message.get(request_id);

    cout << "----- Receive NOTICE -----" << endl;
    cout << "request_id: " << request_id << endl;
    cout << "text: " << text << endl;
}

/**
 * @brief Server Receiving Rawdata from Database
 */
void FIXInitiator::onMessage(const FIX50SP2::Quote& message, const FIX::SessionID&) // override
{
    FIX::QuoteType ordType; // 0 for quote / 1 for trade
    message.get(ordType);

    FIX::NoPartyIDs numOfPartyGroup;
    message.get(numOfPartyGroup);

    if ((!ordType && numOfPartyGroup < 2) || numOfPartyGroup < 1) {
        cout << "Cant find enough Group: NoPartyIDs\n";
        return;
    }

    FIX50SP2::Quote::NoPartyIDs partyGroup1;
    message.getGroup(1, partyGroup1);
    FIX::PartyID buyerID;
    partyGroup1.getField(buyerID);

    FIX::Symbol symbol;
    FIX::BidPx bidPx;
    FIX::BidSize bidSize;
    FIX::TransactTime transactTime;

    message.get(symbol);
    message.get(bidPx);
    message.get(bidSize);
    message.get(transactTime);

    std::map<std::string, Stock>::iterator stockIt;
    stockIt = stocklist.find(symbol);
    if (stockIt == stocklist.end()) {
        cout << "Receive error in Global" << endl; // orderQty=0;
        return;
    }

    Quote newquote(symbol, bidPx, bidSize / 100, buyerID, transactTime.getValue());
    newquote.setordertype('7'); // Update as "trade" from global;

    long mili = timepara.past_milli(transactTime.getValue());
    newquote.setmili(mili);

    if (!ordType) { // Quote
        FIX50SP2::Quote::NoPartyIDs partyGroup2;
        message.getGroup(2, partyGroup2);
        FIX::PartyID sellerID;
        partyGroup2.getField(sellerID);

        FIX::OfferPx offerPx;
        FIX::OfferSize offerSize;
        message.get(offerPx);
        message.get(offerSize);

        Quote newquote2(symbol, offerPx, offerSize, sellerID, transactTime.getValue());
        // Quote(string stockname1, double price1, int size1, string destination1,string time1)
        newquote2.setordertype('8'); // Update global ask
        newquote2.setmili(mili);
        newquote.setsize(bidSize);
        newquote.setordertype('9'); // Update global bid;

        stockIt->second.buf_new_global(newquote2);
        // cout<<"time: "<<(*it).first<<"   data: "<<(*it).second.getstockname()<<"  "<<(*it).second.gettime()<<"  "<<(*it).second.getprice()<<"  "<<(*it).second.getordertype()<<endl;
    }
    stockIt->second.buf_new_global(newquote);
}
