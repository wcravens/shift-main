/*
** Connector to MatchingEngine
**/

#include "FIXAcceptor.h"

#include "DBFIXDataCarrier.h"
#include "PSQL.h"
#include "RequestsProcessorPerTarget.h"

#include <iomanip>
#include <sstream>

#include <shift/miscutils/terminal/Common.h>

// Predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1"); // FIX BEGIN STRING

/*static*/ std::string FIXAcceptor::s_senderID;

FIXAcceptor::FIXAcceptor()
{
    if (!PSQLManager::getInstance().connectDB()) {
        cout << COLOR_ERROR "ERROR: FIXAcceptor fails to connect DB." NO_COLOR << endl;
    }
}

/**
 * @brief This will automatically disconnects the 
 *        FIX channel and PostgreSQL.
 */
FIXAcceptor::~FIXAcceptor() // override
{
    disconnectMatchingEngine();
    PSQLManager::getInstance().disconnectDB();
    /* All request processors of all targets (1 for each) are terminated and destroyed here */
}

/*static*/ FIXAcceptor* FIXAcceptor::getInstance()
{
    static FIXAcceptor s_FIXAcceptor;
    return &s_FIXAcceptor;
}

/**
 * @brief Initiates a FIX session as an acceptor.
 * @param configFile: The session settings file's full path.
 */
void FIXAcceptor::connectMatchingEngine(const std::string& configFile, bool verbose)
{
    disconnectMatchingEngine();

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

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

/** 
 * @brief Ends the FIX session as the acceptor. 
 */
void FIXAcceptor::disconnectMatchingEngine()
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
 * @brief Sends trade or quote data to ME.
 */
/* static */ void FIXAcceptor::sendRawData(const RawData& rawData, const std::string& targetID)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(FIXAcceptor::s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_Quote));

    time_t secs = rawData.secs + static_cast<int>(rawData.microsecs);
    int millisec = static_cast<int>((rawData.microsecs - static_cast<int>(rawData.microsecs)) * 1000000);
    auto utcTime = FIX::UtcTimeStamp(secs, millisec, 6);

    message.setField(FIX::TransactTime(utcTime, 6));
    message.setField(FIX::Symbol(rawData.symbol));
    message.setField(FIX::SecurityID(rawData.toq));

    if ('Q' == rawData.toq.front()) {
        message.setField(FIX::QuoteID(rawData.buyerID));
        message.setField(FIX::BidPx(rawData.bidPrice));
        message.setField(FIX::BidSize(rawData.bidSize));
        message.setField(FIX::QuoteReqID(rawData.sellerID));
        message.setField(FIX::OfferSize(rawData.askSize));
        message.setField(FIX::OfferPx(rawData.askPrice));
    } else if ('T' == rawData.toq.front()) {
        message.setField(FIX::QuoteID(rawData.exchangeID));
        message.setField(FIX::BidPx(rawData.price));
        message.setField(FIX::BidSize(rawData.volume));
    }

    FIX::Session::sendToTarget(message);
}

/**
 * @brief For notifying ME requested works were done
 * 
 * @param text: Indicate the meaning/type of the notice
 */
/* static */ void FIXAcceptor::sendNotice(const std::string& text, const std::string& requestID, const std::string& targetID)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(FIXAcceptor::s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_News));

    // The RequestID of requested job
    message.setField(FIX::Headline(requestID));
    // Notice message
    FIX50SP2::News::NoLinesOfText textGroup;
    textGroup.set(FIX::Text(text));
    message.addGroup(textGroup);

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    cout << "FIX:Create - " << sessionID << endl;
    FIXAcceptor::s_senderID = sessionID.getSenderCompID();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    cout << '\n'
         << "FIX:Logon - " << sessionID << "\n\t"
         << sessionID.getTargetCompID() << endl;
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << '\n'
         << "FIX:Logout - " << sessionID << endl;
}

void FIXAcceptor::toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) // override
{
    try {
        FIX::PossDupFlag possDupFlag;
        message.getHeader().getField(possDupFlag);
        if (possDupFlag)
            throw FIX::DoNotSend();
    } catch (FIX::FieldNotFound&) {
    }
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief Receiving one action record and save to 
 *        database as one trading record.
 */
void FIXAcceptor::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoPartyIDs numOfPartyGroup;
    message.get(numOfPartyGroup);
    if (numOfPartyGroup < 3) {
        cout << "Cant find Group: NoPartyIDs\n";
        return;
    }

    FIX::NoTrdRegTimestamps numOfTimeGroup;
    message.get(numOfTimeGroup);
    if (numOfTimeGroup < 2) {
        cout << "Cant find Group: NoTrdRegTimestamps\n";
        return;
    }

    FIX::OrderID orderID1;
    FIX::SecondaryOrderID orderID2;
    FIX::OrdStatus decision;
    FIX::Symbol symbol;
    FIX::Side orderType1;
    FIX::OrdType orderType2;
    FIX::CumQty size;
    FIX::Price price;
    FIX::TransactTime serverTime;
    FIX::EffectiveTime utc_exetime;

    FIX50SP2::ExecutionReport::NoPartyIDs partyGroup;
    FIX::PartyID traderID1;
    FIX::PartyID traderID2;
    FIX::PartyID destination;

    FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup;
    FIX::TrdRegTimestamp utc_time1;
    FIX::TrdRegTimestamp utc_time2;

    message.get(orderID1);
    message.get(orderID2);
    message.get(decision);
    message.get(symbol);
    message.get(orderType1);
    message.get(orderType2);
    message.get(size);
    message.get(price);
    message.get(serverTime);
    message.get(utc_exetime);

    message.getGroup(1, partyGroup);
    partyGroup.get(traderID1);
    message.getGroup(2, partyGroup);
    partyGroup.get(traderID2);
    message.getGroup(3, partyGroup);
    partyGroup.get(destination);

    message.getGroup(1, timeGroup);
    timeGroup.get(utc_time1);
    message.getGroup(2, timeGroup);
    timeGroup.get(utc_time2);

    // Insert to trading_records failed
    if (decision != '4') { // decision == 4, means this is a trade update from TRTH, no need to save this.
        TradingRecord trade{
            serverTime.getValue(),
            utc_exetime.getValue(),
            std::string(symbol),
            double(price),
            int(size),
            std::string(traderID1),
            std::string(traderID2),
            std::string(orderID1),
            std::string(orderID2),
            char(orderType1),
            char(orderType2),
            char(decision),
            std::string(destination),
            utc_time1.getValue(),
            utc_time2.getValue()
        };
        PSQLManager::getInstance().insertTradingRecord(trade);
    }
}

/** 
 * @brief receive the market data request from ME
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    const std::string targetID = sessionID.getTargetCompID();

    if (m_requestsProcessors.count(targetID)) {
        m_requestsProcessors[targetID]->enqueueNextDataRequest();
    } else {
        cout << COLOR_ERROR "ERROR: No market data request from " << targetID << NO_COLOR << endl;
        return;
    }
}

/** 
 * @brief The FIX message handler of the acceptor.
 *        Identifies message types and push to processor accordingly.
 */
void FIXAcceptor::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    const std::string targetID = sessionID.getTargetCompID();

    if (m_requestsProcessors.count(targetID) == 0) {
        m_requestsProcessors[targetID].reset(new RequestsProcessorPerTarget(targetID)); // Spawn an unique processing thread for the target
    }

    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);

    if (numOfGroup < 1) {
        cout << "Cannot find any Symbol in SecurityList\n";
        return;
    }

    FIX::SecurityResponseID request_id;
    FIX::SecurityListID start_time_s;
    FIX::SecurityListRefID end_time_s;

    message.get(request_id);
    message.get(start_time_s);
    message.get(end_time_s);

    boost::posix_time::ptime start_time = boost::posix_time::from_iso_string(start_time_s);
    boost::posix_time::ptime end_time = boost::posix_time::from_iso_string(end_time_s);
    cout << "Request info:" << '\n'
         << start_time_s << '\n'
         << end_time_s << '\n'
         << request_id << endl;

    FIX::Symbol symbol;
    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    std::vector<std::string> symbols;

    for (int i = 1; i <= numOfGroup; ++i) {
        message.getGroup(i, relatedSymGroup);
        relatedSymGroup.get(symbol);
        cout << i << ":\t" << symbol << endl;
        symbols.push_back(symbol);
    }
    cout << endl;

    m_requestsProcessors[targetID]->enqueueMarketDataRequest(std::move(request_id), std::move(symbols), std::move(start_time), std::move(end_time));
}
