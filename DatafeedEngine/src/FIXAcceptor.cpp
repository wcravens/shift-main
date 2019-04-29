/*
** Connector to MatchingEngine
**/

#include "FIXAcceptor.h"

#include "PSQL.h"
#include "RawData.h"
#include "RequestsProcessorPerTarget.h"

#include <atomic>
#include <cassert>
#include <iomanip>
#include <sstream>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

// Predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_PARTYROLE_EXECUTION_VENUE = FIX::PartyRole(FIX::PartyRole_EXECUTION_VENUE);

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
 * @brief For notifying ME requested works were done
 *
 * @param text: Indicate the meaning/type of the notice
 */
/* static */ void FIXAcceptor::sendNotice(const std::string& targetID, const std::string& requestID, const std::string& text)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
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

/**
 * @brief Sends trade or quote data to ME.
 */
/* static */ void FIXAcceptor::sendRawData(const std::string& targetID, const RawData& rawData)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(FIXAcceptor::s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_Quote));

    // tm is always in local time and std::gmtime transforms time_t in tm disregarding timezone,
    // since it assumes time_t is always in UTC (ours is in New York time)
    std::time_t secs = rawData.secs + static_cast<int>(rawData.microsecs);
    std::tm* secs_tm = std::gmtime(&secs);

    // A negative value of time->tm_isdst causes mktime to attempt to determine if DST was in effect
    // More information is available at: https://en.cppreference.com/w/cpp/chrono/c/mktime
    secs_tm->tm_isdst = -1;
    std::time_t utcSecs = std::mktime(secs_tm);

    int millisec = static_cast<int>((rawData.microsecs - static_cast<int>(rawData.microsecs)) * 1000000);

    message.setField(FIX::QuoteID(shift::crossguid::newGuid().str()));
    message.setField(FIX::QuoteType(rawData.toq.front() == 'Q' ? 0 : 1));
    message.setField(FIX::Symbol(rawData.symbol));
    message.setField(FIX::TransactTime(FIX::UtcTimeStamp(utcSecs, millisec, 6), 6));

    FIX50SP2::Quote::NoPartyIDs idGroup1;
    idGroup1.setField(FIXFIELD_PARTYROLE_EXECUTION_VENUE);
    idGroup1.setField( // keep the order of idGroup1 and idGroup2
        'Q' == rawData.toq.front() ? FIX::PartyID(rawData.buyerID) : FIX::PartyID(rawData.exchangeID));
    message.addGroup(idGroup1);

    if ('Q' == rawData.toq.front()) {
        FIX50SP2::Quote::NoPartyIDs idGroup2;
        idGroup2.setField(FIXFIELD_PARTYROLE_EXECUTION_VENUE);
        idGroup2.setField(FIX::PartyID(rawData.sellerID));
        message.addGroup(idGroup2);

        message.setField(FIX::BidPx(rawData.bidPrice));
        message.setField(FIX::OfferPx(rawData.askPrice));
        message.setField(FIX::BidSize(rawData.bidSize));
        message.setField(FIX::OfferSize(rawData.askSize));
    } else if ('T' == rawData.toq.front()) {
        message.setField(FIX::BidPx(rawData.price));
        message.setField(FIX::BidSize(rawData.volume));
    }

    FIX::Session::sendToTarget(message);
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    cout << "FIX:Create - " << sessionID << endl;
    FIXAcceptor::s_senderID = sessionID.getSenderCompID().getValue();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    cout << '\n'
         << "FIX:Logon - " << sessionID << "\n\t"
         << sessionID.getTargetCompID().getValue() << endl;
}

void FIXAcceptor::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << '\n'
         << "FIX:Logout - " << sessionID.toString() << endl;
}

void FIXAcceptor::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief The FIX message handler of the acceptor.
 *        Identifies message types and push to processor accordingly.
 */
void FIXAcceptor::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    const std::string targetID = sessionID.getTargetCompID().getValue();

    if (m_requestsProcessors.count(targetID) == 0) {
        m_requestsProcessors[targetID].reset(new RequestsProcessorPerTarget(targetID)); // Spawn an unique processing thread for the target
    }

    static FIX::SecurityResponseID requestID;
    static FIX::SecurityListID startTimeString;
    static FIX::SecurityListRefID endTimeString;

    static FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SecurityResponseID* pRequestID;
    FIX::SecurityListID* pStartTimeString;
    FIX::SecurityListRefID* pEndTimeString;

    FIX50SP2::SecurityList::NoRelatedSym* pRelatedSymGroup;
    FIX::Symbol* pSymbol;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pRequestID = &requestID;
        pStartTimeString = &startTimeString;
        pEndTimeString = &endTimeString;
        pRelatedSymGroup = &relatedSymGroup;
        pSymbol = &symbol;
    } else { // > 1 threads; always safe way:
        pRequestID = new decltype(requestID);
        pStartTimeString = new decltype(startTimeString);
        pEndTimeString = new decltype(endTimeString);
        pRelatedSymGroup = new decltype(relatedSymGroup);
        pSymbol = new decltype(symbol);
    }

    message.get(*pRequestID);
    message.get(*pStartTimeString);
    message.get(*pEndTimeString);

    cout << "Request info:" << '\n'
         << pRequestID->getValue() << '\n'
         << pStartTimeString->getValue() << '\n'
         << pEndTimeString->getValue() << endl;

    std::vector<std::string> symbols;

    for (int i = 1; i <= numOfGroups.getValue(); ++i) {
        message.getGroup(static_cast<unsigned int>(i), *pRelatedSymGroup);
        pRelatedSymGroup->get(*pSymbol);
        cout << i << ":\t" << pSymbol->getValue() << endl;
        symbols.push_back(pSymbol->getValue());
    }
    cout << endl;

    boost::posix_time::ptime startTime = boost::posix_time::from_iso_string(*pStartTimeString);
    boost::posix_time::ptime endTime = boost::posix_time::from_iso_string(*pEndTimeString);

    m_requestsProcessors[targetID]->enqueueMarketDataRequest(*pRequestID, std::move(symbols), std::move(startTime), std::move(endTime));

    if (prevCnt) { // > 1 threads
        delete pRequestID;
        delete pStartTimeString;
        delete pEndTimeString;
        delete pRelatedSymGroup;
        delete pSymbol;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief receive the market data request from ME
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    const std::string targetID = sessionID.getTargetCompID().getValue();

    if (m_requestsProcessors.count(targetID)) {
        m_requestsProcessors[targetID]->enqueueNextDataRequest();
    } else {
        cout << COLOR_ERROR "ERROR: No market data request from " << targetID << NO_COLOR << endl;
        return;
    }
}
