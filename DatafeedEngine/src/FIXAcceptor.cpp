#include "FIXAcceptor.h"

#include "PSQL.h"
#include "Parameters.h"
#include "RawData.h"
#include "RequestsProcessorPerTarget.h"

#include <atomic>
#include <cassert>
#include <iomanip>
#include <sstream>

#include <quickfix/FieldConvertors.h>
#include <quickfix/FieldTypes.h>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>

// predefined constant FIX message fields (to avoid recalculations):
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
 * @brief This will automatically disconnects the FIX channel and PostgreSQL.
 */
FIXAcceptor::~FIXAcceptor() // override
{
    disconnectMatchingEngine();
    PSQLManager::getInstance().disconnectDB();
    // all request processors of all targets (1 for each) are terminated and destroyed here
}

/*static*/ FIXAcceptor* FIXAcceptor::getInstance()
{
    static FIXAcceptor s_FIXAcceptor;
    return &s_FIXAcceptor;
}

/**
 * @brief Initiates a FIX session as an acceptor.
 * @param configFile The session settings file's full path.
 */
bool FIXAcceptor::connectMatchingEngine(const std::string& configFile, bool verbose, const std::string& cryptoKey, const std::string& dbConfigFile)
{
    disconnectMatchingEngine();

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
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLLogDatabase")) { // store all log events into database
        auto loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, dbConfigFile);
        commonDict.setString("PostgreSQLLogUser", loginInfo["DBUser"]);
        commonDict.setString("PostgreSQLLogPassword", loginInfo["DBPassword"]);
        commonDict.setString("PostgreSQLLogHost", loginInfo["DBHost"]);
        commonDict.setString("PostgreSQLLogPort", loginInfo["DBPort"]);
        settings.set(commonDict);
        m_logFactoryPtr.reset(new FIX::PostgreSQLLogFactory(settings));
#endif
    } else { // display all log events onto the standard output
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, verbose)); // incoming, outgoing, event
    }

    if (commonDict.has("FileStorePath")) { // store all outgoing messages into flat files
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLStoreDatabase")) { // store all outgoing messages into database
        auto loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, dbConfigFile);
        commonDict.setString("PostgreSQLStoreUser", loginInfo["DBUser"]);
        commonDict.setString("PostgreSQLStorePassword", loginInfo["DBPassword"]);
        commonDict.setString("PostgreSQLStoreHost", loginInfo["DBHost"]);
        commonDict.setString("PostgreSQLStorePort", loginInfo["DBPort"]);
        settings.set(commonDict);
        m_messageStoreFactoryPtr.reset(new FIX::PostgreSQLStoreFactory(settings));
#endif
    } else { // store all outgoing messages in memory
        m_messageStoreFactoryPtr.reset(new FIX::MemoryStoreFactory());
    }
    // } else { // do not store messages
    //     m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    // }

    m_acceptorPtr.reset(new FIX::SocketAcceptor(*getInstance(), *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

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

/* static */ inline double FIXAcceptor::s_roundNearest(double value, double nearest)
{
    return std::round(value / nearest) * nearest;
}

/**
 * @brief For notifying ME requested works were done.
 * @param text Indicate the meaning/type of the notice
 */
/* static */ void FIXAcceptor::sendNotice(const std::string& targetID, const std::string& requestID, const std::string& text)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(FIXAcceptor::s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_News));

    // the request ID of requested job
    message.setField(FIX::Headline(requestID));

    // notice message
    shift::fix::addFIXGroup<FIX50SP2::News::NoLinesOfText>(message,
        FIX::Text(text));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Sends trade or quote data to ME.
 */
/* static */ void FIXAcceptor::sendRawData(const std::string& targetID, const std::vector<RawData>& rawData)
{
    for (const auto& rd : rawData) {
        if (('T' == rd.toq.front()) && (rd.volume < 100)) {
            continue;
        }

        FIX::Message message;

        FIX::Header& header = message.getHeader();
        header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
        header.setField(FIX::SenderCompID(FIXAcceptor::s_senderID));
        header.setField(FIX::TargetCompID(targetID));
        header.setField(FIX::MsgType(FIX::MsgType_Quote));

        // tm is always in local time and std::gmtime transforms time_t in tm disregarding timezone,
        // since it assumes time_t is always in UTC (ours is in New York time)
        std::time_t secs = rd.secs + static_cast<int>(rd.microsecs);
        std::tm* secs_tm = std::gmtime(&secs);

        // a negative value of time->tm_isdst causes mktime to attempt to determine if DST was in effect --
        // more information is available at: https://en.cppreference.com/w/cpp/chrono/c/mktime
        secs_tm->tm_isdst = -1;
        std::time_t utcSecs = std::mktime(secs_tm);

        int millisec = static_cast<int>((rd.microsecs - static_cast<int>(rd.microsecs)) * 1000000);

        message.setField(FIX::QuoteID(shift::crossguid::newGuid().str()));
        message.setField(FIX::QuoteType(rd.toq.front() == 'Q' ? 0 : 1));
        message.setField(FIX::Symbol(rd.symbol));
        message.setField(FIX::TransactTime(FIX::UtcTimeStamp(utcSecs, millisec, 6), 6));

        shift::fix::addFIXGroup<FIX50SP2::Quote::NoPartyIDs>(message,
            FIXFIELD_PARTYROLE_EXECUTION_VENUE,
            'Q' == rd.toq.front() ? FIX::PartyID(rd.buyerID) : FIX::PartyID(rd.exchangeID));

        if ('Q' == rd.toq.front()) {
            shift::fix::addFIXGroup<FIX50SP2::Quote::NoPartyIDs>(message,
                FIXFIELD_PARTYROLE_EXECUTION_VENUE,
                FIX::PartyID(rd.sellerID));

            message.setField(FIX::BidPx(rd.bidPrice));
            message.setField(FIX::OfferPx(rd.askPrice));
            message.setField(FIX::BidSize(rd.bidSize));
            message.setField(FIX::OfferSize(rd.askSize));
        } else { // if ('T' == rd.toq.front())
            message.setField(FIX::BidPx(FIXAcceptor::s_roundNearest(rd.price, 0.01)));
            message.setField(FIX::BidSize(rd.volume / 100)); // this is and *should be* an int division
        }

        FIX::Session::sendToTarget(message);
    }
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
 * @brief The FIX message handler of the acceptor. Identifies message types and push to processor accordingly.
 */
void FIXAcceptor::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    const std::string targetID = sessionID.getTargetCompID().getValue();

    std::unique_lock<std::mutex> lockRP(m_mtxReqsProcs);
    if (m_requestsProcessorByTarget.find(targetID) == m_requestsProcessorByTarget.end()) {
        m_requestsProcessorByTarget[targetID].reset(new RequestsProcessorPerTarget(targetID)); // spawn an unique processing thread for the target
    }
    lockRP.unlock();

    static FIX::SecurityResponseID requestID;
    static FIX::SecurityListID startTimeString;
    static FIX::SecurityListRefID endTimeString;
    static FIX::SecurityListDesc dataChunkPeriod;

    static FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX::SecurityResponseID* pRequestID;
    FIX::SecurityListID* pStartTimeString;
    FIX::SecurityListRefID* pEndTimeString;
    FIX::SecurityListDesc* pDataChunkPeriod;

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
        pDataChunkPeriod = &dataChunkPeriod;
        pRelatedSymGroup = &relatedSymGroup;
        pSymbol = &symbol;
    } else { // > 1 threads; always safe way:
        pRequestID = new decltype(requestID);
        pStartTimeString = new decltype(startTimeString);
        pEndTimeString = new decltype(endTimeString);
        pDataChunkPeriod = new decltype(dataChunkPeriod);
        pRelatedSymGroup = new decltype(relatedSymGroup);
        pSymbol = new decltype(symbol);
    }

    message.getField(*pRequestID);
    message.getField(*pStartTimeString);
    message.getField(*pEndTimeString);
    message.getField(*pDataChunkPeriod);

    cout << "Request info:" << '\n'
         << pRequestID->getValue() << '\n'
         << pStartTimeString->getValue() << '\n'
         << pEndTimeString->getValue() << endl;

    std::vector<std::string> symbols;

    for (int i = 1; i <= numOfGroups.getValue(); ++i) {
        message.getGroup(static_cast<unsigned int>(i), *pRelatedSymGroup);
        pRelatedSymGroup->getField(*pSymbol);

        std::string symbol = pSymbol->getValue();
        ::cvtRICToDEInternalRepresentation(&symbol);

        cout << i << ":\t" << symbol << endl;
        symbols.push_back(symbol);
    }
    cout << endl;

    boost::posix_time::ptime startTime = boost::posix_time::from_iso_string(*pStartTimeString);
    boost::posix_time::ptime endTime = boost::posix_time::from_iso_string(*pEndTimeString);

    const int numSecondsPerDataChunk = std::stoi(pDataChunkPeriod->getValue());

    lockRP.lock();
    m_requestsProcessorByTarget[targetID]->enqueueMarketDataRequest(std::move(*pRequestID), std::move(symbols), std::move(startTime), std::move(endTime), numSecondsPerDataChunk);
    lockRP.unlock();

    if (prevCnt) { // > 1 threads
        delete pRequestID;
        delete pStartTimeString;
        delete pEndTimeString;
        delete pDataChunkPeriod;
        delete pRelatedSymGroup;
        delete pSymbol;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Receive the market data request from ME.
 */
void FIXAcceptor::onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) // override
{
    const std::string& targetID = sessionID.getTargetCompID().getValue();

    std::lock_guard<std::mutex> guard(m_mtxReqsProcs);
    if (m_requestsProcessorByTarget.find(targetID) != m_requestsProcessorByTarget.end()) {
        m_requestsProcessorByTarget[targetID]->enqueueNextDataRequest();
    } else {
        cout << COLOR_ERROR "ERROR: No security list ever requested by the target [" << targetID << "] hence no market data chunk to send!" NO_COLOR << endl;
        return;
    }
}

/**
 * @brief Receive the confirmed order from ME.
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) // override
{
    FIX::ClOrdID orderID;
    FIX::Symbol symbol;
    FIX::OrderQty size;
    FIX::OrdType type;
    FIX::Price price;
    FIX::PartyID traderID;

    FIX50SP2::NewOrderSingle::NoPartyIDs g;
    message.getGroup(1, g);
    g.getField(traderID);

    message.getField(orderID);
    message.getField(symbol);
    message.getField(size);
    message.getField(type);
    message.getField(price);
    message.getField(traderID);

    cout << "Confirmed Order:" << '\n'
         << orderID << '\n'
         << symbol << '\n'
         << size << '\n'
         << type << '\n'
         << price << '\n'
         << traderID << endl;
}
