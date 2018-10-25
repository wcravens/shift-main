#include "StockSummary.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <boost/date_time.hpp>
#include <boost/format.hpp>

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

StockSummary::StockSummary()
    : m_currPrice{ .0 }
    , m_currOpenPrice{ .0 }
    , m_currClosePrice{ .0 }
    , m_currHighPrice{ .0 }
    , m_currLowPrice{ .0 }
    , m_sent(true)
{
}

StockSummary::StockSummary(std::string symbol, double currPrice, double currOpenPrice, double currClosePrice, double currHighPrice, double currLowPrice, std::time_t currOpenTime)
    : m_symbol(std::move(symbol))
    , m_currPrice{ currPrice }
    , m_currOpenPrice{ currOpenPrice }
    , m_currClosePrice{ currClosePrice }
    , m_currHighPrice{ currHighPrice }
    , m_currLowPrice{ currLowPrice }
    , m_currOpenTime(currOpenTime)
    , m_sent(true)
{
}

StockSummary::~StockSummary()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvSS, *m_th);
    m_th = nullptr;
}

const std::string& StockSummary::getSymbol() const
{
    return m_symbol;
}

/*static*/ std::time_t StockSummary::s_nowUnixTimestamp() noexcept
{
    auto tnow = boost::posix_time::to_tm(boost::posix_time::microsec_clock::local_time());
    return std::mktime(&tnow);
}

/*static*/ std::time_t StockSummary::s_toUnixTimestamp(const std::string& time) noexcept
{
    struct std::tm tm;
    ::strptime(time.c_str(), CSTR_TIME_FORMAT_YMDHMS, &tm);
    std::time_t t = std::mktime(&tm);
    return t;
}

void StockSummary::enqueueTransaction(const Transaction& t)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxTransacBuff);
        m_transacBuff.push(t);
        m_tranBufSizeAtom = m_transacBuff.size();
    }
    m_cvSS.notify_one();
}

void StockSummary::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    auto writeStockSummaryHistory = [this](const TempStockSummary& tempSS) {
                            std::lock_guard<std::mutex> guard(m_mtxHistory);
                            m_history[m_currOpenTime] = tempSS;
                        };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxTransacBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvSS, lock, [this] { return !m_transacBuff.empty(); }))
            return;

        const auto transac = m_transacBuff.front();
        m_transacBuff.pop();
        m_tranBufSizeAtom = m_transacBuff.size();
        lock.unlock(); // now it's safe to access the front element because the list contains at least 2 elements

        const auto texec = transac.utc_execTime.getTimeT();
        // cout << "Test Use: " << texec << " " << m_currOpenTime << endl;
        const auto tdiff = texec - m_currOpenTime;

        if (m_sent) {
            DEBUG_PERFORMANCE_COUNTER(pc1, "ifsent")

            if (tdiff >= 1) {
                DEBUG_PERFORMANCE_COUNTER(pc2, "ifsentsend")

                for (decltype(tdiff - 0) _i{ 1 }; _i < tdiff; _i++) {
                    m_currPrice
                        = m_currOpenPrice
                        = m_currHighPrice
                        = m_currLowPrice
                        = m_currClosePrice;

                    m_currOpenTime += 1;

                    TempStockSummary tempSS(m_symbol, m_currClosePrice, m_currClosePrice, m_currClosePrice, m_currClosePrice, m_currOpenTime);
                    sendCurrentStockSummary(tempSS);

                    writeStockSummaryHistory(tempSS);
                }
            }

            m_currPrice
                = m_currOpenPrice
                = m_currClosePrice
                = m_currHighPrice
                = m_currLowPrice
                = transac.price;

            m_currOpenTime = texec;
            m_sent = false;

            const auto topn = s_nowUnixTimestamp();

            while (1 == m_tranBufSizeAtom) {
                if (s_nowUnixTimestamp() - topn <= 1) {
                    DEBUG_PERFORMANCE_COUNTER_S(pc3, "###########", 100000);
                    continue;
                }

                DEBUG_PERFORMANCE_COUNTER_S(pc4, "%%%%%%%%%%%", 1);

                m_currClosePrice = m_currPrice;

                writeStockSummaryHistory({m_symbol, m_currOpenPrice, m_currClosePrice, m_currHighPrice, m_currLowPrice, m_currOpenTime});

                m_sent = true;
                break;
            }

            continue; // process next transaction
        } else if (tdiff <= 1) {
            DEBUG_PERFORMANCE_COUNTER(pc5, "notsent1")

            m_currClosePrice = m_currPrice = transac.price;
            m_currLowPrice = (m_currPrice <= m_currLowPrice ? m_currPrice : m_currLowPrice); // min
            m_currHighPrice = (m_currPrice >= m_currHighPrice ? m_currPrice : m_currHighPrice); // max
        } else { // > 1
            DEBUG_PERFORMANCE_COUNTER(pc6, "notsent2")

            TempStockSummary tempSS(m_symbol, m_currOpenPrice, m_currClosePrice, m_currHighPrice, m_currLowPrice, m_currOpenTime);
            sendCurrentStockSummary(tempSS);

            writeStockSummaryHistory(tempSS);

            m_sent = true;
        }
    }
}

void StockSummary::spawn()
{
    m_th.reset(new std::thread(&StockSummary::process, this));
}

void StockSummary::sendCurrentStockSummary(const TempStockSummary& tempSS)
{
    std::lock_guard<std::mutex> guard(m_mtxSSClientList);
    for (const auto& userName : m_clientList)
        FIXAcceptor::instance()->sendTempStockSummary(userName, tempSS);
}

void StockSummary::sendHistory(const std::string userName)
{
    std::map<std::time_t, TempStockSummary> history;
    {
        std::lock_guard<std::mutex> guard(m_mtxHistory);
        history = m_history;
    }
    for (const auto& i : history) {
        FIXAcceptor::instance()->sendTempStockSummary(userName, i.second);
    }
}

void StockSummary::registerClientInSS(const std::string& userName)
{
    std::thread(&StockSummary::sendHistory, this, userName).detach();
    {
        std::lock_guard<std::mutex> guard(m_mtxSSClientList);
        m_clientList.insert(userName);
    }
    BCDocuments::instance()->addCandleSymbolToClient(userName, m_symbol);
}

void StockSummary::unregisterClientInSS(const std::string& userName)
{
    std::lock_guard<std::mutex> guard(m_mtxSSClientList);
    auto it = m_clientList.find(userName);
    if (it != m_clientList.end()) {
        m_clientList.erase(it);
    }
}

//----------------------------------------------------------------------------

// /*static*/ std::string StockSummary::s_toNormalTime(std::time_t t)
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
