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

CandlestickData::CandlestickData(std::string symbol, double currPrice, double currOpenPrice, double currClosePrice, double currHighPrice, double currLowPrice, std::time_t currOpenTime)
    : m_symbol(std::move(symbol))
    , m_currPrice{ currPrice }
    , m_currOpenPrice{ currOpenPrice }
    , m_currClosePrice{ currClosePrice }
    , m_currHighPrice{ currHighPrice }
    , m_currLowPrice{ currLowPrice }
    , m_currOpenTime(currOpenTime)
    , m_lastTransacSent(true)
{
}

CandlestickData::~CandlestickData()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvCD, *m_th);
    m_th = nullptr;
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

void CandlestickData::enqueueTransaction(const Transaction& t)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxTransacBuff);
        m_transacBuff.push(t);
        m_tranBufSizeAtom = m_transacBuff.size();
    }
    m_cvCD.notify_one();
}

void CandlestickData::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    auto writeCandlestickDataHistory = [this](const TempCandlestickData& tmpCandle) {
        std::lock_guard<std::mutex> guard(m_mtxHistory);
        m_history[m_currOpenTime] = tmpCandle;
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxTransacBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvCD, lock, [this] { return !m_transacBuff.empty(); }))
            return;

        const auto transac = m_transacBuff.front();
        m_transacBuff.pop();
        m_tranBufSizeAtom = m_transacBuff.size();
        lock.unlock(); // now it's safe to access the front element because the list contains at least 2 elements

        const auto currExecTime = transac.execTime.getTimeT();

        if (std::time_t{} == m_currOpenTime || m_symbol.empty()) { // is Candlestick Data not yet initialized to valid startup status ?
            m_symbol = std::move(transac.symbol);
            m_currPrice
                = m_currOpenPrice
                = m_currClosePrice
                = m_currHighPrice
                = m_currLowPrice
                = transac.price;
            m_currOpenTime = currExecTime;
            continue;
        }

        const auto execTimeGap = currExecTime - m_currOpenTime;

        if (m_lastTransacSent) {
            DEBUG_PERFORMANCE_COUNTER(pc1, "ifsent")

            if (execTimeGap > 1) {
                DEBUG_PERFORMANCE_COUNTER(pc2, "ifsentsend")

                // second-wise-send the data between the gap time range
                for (int cnt = 1; cnt < execTimeGap; cnt++) {
                    m_currPrice
                        = m_currOpenPrice
                        = m_currHighPrice
                        = m_currLowPrice
                        = m_currClosePrice;

                    m_currOpenTime++;

                    TempCandlestickData tmpCandle(m_symbol, m_currClosePrice, m_currClosePrice, m_currClosePrice, m_currClosePrice, m_currOpenTime);
                    sendCurrentCandlestickData(tmpCandle);
                    writeCandlestickDataHistory(tmpCandle);
                }
            }

            m_currPrice
                = m_currOpenPrice
                = m_currClosePrice
                = m_currHighPrice
                = m_currLowPrice
                = transac.price;

            m_currOpenTime = currExecTime;
            m_lastTransacSent = false; // reset to the state of current transaction

            const auto startTime = s_nowUnixTimestamp(); // begin the timing...
            while (1 == m_tranBufSizeAtom) {
                if (s_nowUnixTimestamp() - startTime > 1) { // elapsed more than 1 sec ?
                    DEBUG_PERFORMANCE_COUNTER_S(pc4, "%%%%%%%%%%%", 1);

                    m_currClosePrice = m_currPrice;
                    writeCandlestickDataHistory({ m_symbol, m_currOpenPrice, m_currClosePrice, m_currHighPrice, m_currLowPrice, m_currOpenTime });
                    m_lastTransacSent = true;

                    break;
                }
                DEBUG_PERFORMANCE_COUNTER_S(pc3, "###########", 100000);
                // timer counting continues...
            }

            continue; // process next transaction immediately
        } else if (execTimeGap <= 1) { // within 1 sec elapsed since last transac ?
            DEBUG_PERFORMANCE_COUNTER(pc5, "notsent1")
            // update current ongoing window

            m_currClosePrice = m_currPrice = transac.price;
            m_currLowPrice = std::min(m_currPrice, m_currLowPrice); // min
            m_currHighPrice = std::max(m_currPrice, m_currHighPrice); // max
        } else { // execTimeGap > 1
            DEBUG_PERFORMANCE_COUNTER(pc6, "notsent2")
            // finish and send current window

            TempCandlestickData tmpCandle(m_symbol, m_currOpenPrice, m_currClosePrice, m_currHighPrice, m_currLowPrice, m_currOpenTime);
            sendCurrentCandlestickData(tmpCandle);
            writeCandlestickDataHistory(tmpCandle);

            m_lastTransacSent = true;
        }
    }
}

void CandlestickData::spawn()
{
    m_th.reset(new std::thread(&CandlestickData::process, this));
}

void CandlestickData::sendCurrentCandlestickData(const TempCandlestickData& tmpCandle)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleUserList);
    FIXAcceptor::getInstance()->sendCandlestickData(m_candleUserList, tmpCandle);
}

void CandlestickData::sendHistory(const std::string userName)
{
    std::map<std::time_t, TempCandlestickData> history;
    {
        std::lock_guard<std::mutex> guard(m_mtxHistory);
        history = m_history;
    }
    for (const auto& i : history) {
        FIXAcceptor::getInstance()->sendCandlestickData({ userName }, i.second);
    }
}

void CandlestickData::registerUserInCandlestickData(const std::string& userName)
{
    std::thread(&CandlestickData::sendHistory, this, userName).detach(); // this takes time, so let run on side
    {
        std::lock_guard<std::mutex> guard(m_mtxCandleUserList);
        m_candleUserList.insert(userName);
    }
}

void CandlestickData::unregisterUserInCandlestickData(const std::string& userName)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleUserList);
    auto it = m_candleUserList.find(userName);
    if (it != m_candleUserList.end()) {
        m_candleUserList.erase(it);
    }
}

//----------------------------------------------------------------------------

// /*static*/ std::string CandlestickData::s_toNormalTime(std::time_t t)
// {
//     struct std::tm* tm = std::localtime(&t);
//     char normalTimeC[20];
//     strftime(normalTimeC, sizeof(normalTimeC), CSTR_TIME_FORMAT_YMDHMS, tm);
//     normalTimeC[19] = '\0';
//     return normalTimeC;
// }

#undef CSTR_TIME_FORMAT_YMDHMS
#undef DEBUG_PERFORMANCE_COUNTER
#undef DEBUG_PERFORMANCE_COUNTER_S
