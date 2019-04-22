#include "MarketDataRequest.h"

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <quickfix/Fields.h>

/**
 * @brief For identifying incoming requests.
 */
enum class REQUEST_TYPE : unsigned char {
    MARKET_DATA,
    NEXT_DATA,
};

/**
 * @brief Processes requests from one specific target. One object for each unique target (distinguished by Target ID).
 */
class RequestsProcessorPerTarget {
public:
    RequestsProcessorPerTarget(const std::string& targetID);
    ~RequestsProcessorPerTarget();

    RequestsProcessorPerTarget(const RequestsProcessorPerTarget&) = delete;
    RequestsProcessorPerTarget(RequestsProcessorPerTarget&&) = delete;

    void enqueueMarketDataRequest(std::string reqID, std::vector<std::string>&& symbols, boost::posix_time::ptime&& startTime, boost::posix_time::ptime&& endTime);
    void enqueueNextDataRequest();

private:
    void processRequests();

    static void s_announceDataReady(const std::string& targetID, const std::string& requestID);

    std::mutex m_mtxRequest; ///> One per target; for guarding all queues
    std::condition_variable m_cvQueues; ///> For events of all queues

    static void s_processRequestNextData(std::future<bool>* const lastDownloadFutPtr, MarketDataRequest* const currMarketRequestPtr, const std::string& targetID);
    static std::future<bool> s_processRequestMarketData(const MarketDataRequest& marketReq, const std::string& targetID);

    const std::string c_targetID;

    std::thread m_proc; ///> The unique Requests Processor for this target
    std::promise<void> m_procQuitFlag; ///> To terminate the Requests Processor

    std::queue<REQUEST_TYPE> m_queueReqTypes; ///> Queue for received requests messages. The messages are identified by REQUEST_TYPE.

    std::queue<MarketDataRequest> m_queueMarketReqs; ///> Special queue for storing received Market Data requests
};
