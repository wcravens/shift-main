#include "CandlestickData.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <boost/date_time.hpp>

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/terminal/Common.h>

#define CSTR_TIME_FORMAT_YMDHMS \
    "%Y-%m-%d %H:%M:%S"

#if 0
#define DEBUG_PERFORMANCE_COUNTER(_NAME, _LABEL)  \
    {                                             \
        static long long int _NAME = 0;           \
        if (++_NAME % 100 == 0)                   \
            cout << _LABEL ": " << _NAME << endl; \
    }
#define DEBUG_PERFORMANCE_COUNTER_S(_NAME, _LABEL, _STRIDE) \
    {                                                       \
        static long long int _NAME = 0;                     \
        if (++_NAME % _STRIDE == 0)                         \
            cout << _LABEL ": " << _NAME << endl;           \
    }
#else
#define DEBUG_PERFORMANCE_COUNTER(_NAME, _LABEL)
#define DEBUG_PERFORMANCE_COUNTER_S(_NAME, _LABEL, _STRIDE)
#endif

CandlestickData::CandlestickData()
    : CandlestickData("", .0, .0, .0, .0, .0, std::time_t{})
{
}

CandlestickData::CandlestickData(const std::string& symbol, double currPrice, double currOpenPrice, double currClosePrice, double currHighPrice, double currLowPrice, std::time_t currOpenTime)
    : m_symbol(symbol)
    , m_lastOpenPrice(currOpenPrice)
    , m_lastClosePrice(currClosePrice)
    , m_lastHighPrice(currHighPrice)
    , m_lastLowPrice(currLowPrice)
    , m_lastOpenTime(currOpenTime)
{
}

CandlestickData::~CandlestickData() // override
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvCD, *m_th);
    m_th = nullptr;
}

void CandlestickData::sendPoint(const CandlestickDataPoint& cdPoint)
{
    auto targetList = getTargetList();
    if (targetList.empty())
        return;

    FIXAcceptor::getInstance()->sendCandlestickData(targetList, cdPoint);
}

void CandlestickData::sendHistory(std::string targetID)
{
    std::map<std::time_t, CandlestickDataPoint> histCopy;
    {
        std::lock_guard<std::mutex> guard(m_mtxHistory);
        histCopy = m_history;
    }
    const std::vector<std::string> targetList{ std::move(targetID) };

    for (const auto& i : histCopy) {
        FIXAcceptor::getInstance()->sendCandlestickData(targetList, i.second);
    }
}

const std::string& CandlestickData::getSymbol() const
{
    return m_symbol;
}

/*static*/ std::time_t CandlestickData::s_nowUnixTimestamp() noexcept
{
    auto tnow = boost::posix_time::to_tm(boost::posix_time::microsec_clock::local_time());
    return std::mktime(&tnow);
}

/*static*/ std::time_t CandlestickData::s_toUnixTimestamp(const std::string& time) noexcept
{
    struct std::tm tm;
    ::strptime(time.c_str(), CSTR_TIME_FORMAT_YMDHMS, &tm);
    std::time_t t = std::mktime(&tm);
    return t;
}

void CandlestickData::registerUserInCandlestickData(const std::string& targetID)
{
    std::thread(&CandlestickData::sendHistory, this, targetID).detach(); // this takes time, so let run on side
    registerTarget(targetID);
}

void CandlestickData::unregisterUserInCandlestickData(const std::string& targetID)
{
    unregisterTarget(targetID);
}

void CandlestickData::enqueueTransaction(const Transaction& t)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxTransacBuff);
        m_transacBuff.push(t);
    }
    m_cvCD.notify_one();
}

void CandlestickData::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    auto writeCandlestickDataHistory = [this](const CandlestickDataPoint& cdPoint) {
        std::lock_guard<std::mutex> guard(m_mtxHistory);
        m_history[m_lastOpenTime] = cdPoint;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxTransacBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvCD, lock, [this] { return !m_transacBuff.empty(); }))
            return;

        const auto transac = m_transacBuff.front();
        m_transacBuff.pop();
        lock.unlock(); // now it's safe to access the front element because the list contains at least 2 elements

        const auto currSimulTime = transac.simulationTime.getTimeT(); // simulTime in nano-seconds => in seconds; == tick history start timepoint + elapsed simulation duration; see MatchingEngine/src/TimeSetting.cpp for the details.

        if (std::time_t{} == m_lastOpenTime || m_symbol.empty()) { // is Candlestick Data not yet initialized to valid startup state ?
            m_symbol = std::move(transac.symbol);

            m_lastOpenPrice
                = m_lastClosePrice
                = m_lastHighPrice
                = m_lastLowPrice
                = transac.price;

            m_lastOpenTime = currSimulTime;

            continue;
        }

        const auto numGappedSticks = (currSimulTime - m_lastOpenTime) / ::NUM_SECONDS_PER_CANDLESTICK;

        // elapsed at least one candlestick ?:
        if (numGappedSticks >= 1) {
            // send data of every "stalled" preceeding candlesticks, except last one:
            for (int cnt = 0; cnt < numGappedSticks - 1; cnt++) {
                CandlestickDataPoint cdPoint(m_symbol, m_lastClosePrice, m_lastClosePrice, m_lastClosePrice, m_lastClosePrice, m_lastOpenTime);

                sendPoint(cdPoint);
                writeCandlestickDataHistory(cdPoint);

                m_lastOpenTime += ::NUM_SECONDS_PER_CANDLESTICK;
            }

            // for last one:
            CandlestickDataPoint cdPoint(m_symbol, m_lastOpenPrice, m_lastClosePrice, m_lastHighPrice, m_lastLowPrice, m_lastOpenTime);
            sendPoint(cdPoint);
            writeCandlestickDataHistory(cdPoint);

            // initialize for the upcoming new candlestick
            m_lastOpenPrice = m_lastHighPrice = m_lastLowPrice = m_lastClosePrice;
            m_lastOpenTime = currSimulTime;
        }

        // within this new candlestick:
        m_lastClosePrice = transac.price;
        m_lastHighPrice = std::max(m_lastClosePrice, m_lastHighPrice);
        m_lastLowPrice = std::min(m_lastClosePrice, m_lastLowPrice);
    }
}

void CandlestickData::spawn()
{
    m_th.reset(new std::thread(&CandlestickData::process, this));
}

#undef CSTR_TIME_FORMAT_YMDHMS
#undef DEBUG_PERFORMANCE_COUNTER
#undef DEBUG_PERFORMANCE_COUNTER_S
