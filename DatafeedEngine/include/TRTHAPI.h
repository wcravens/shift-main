#pragma once

#include "TRTHRequest.h"

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#define GZ_BUF_LEN (1024 * 10)

/**
 * @brief TRTH 3rd-party SOAP API wrapper and utilities. It is a singleton.
 */
class TRTHAPI {
public:
    static std::atomic<bool> s_bTRTHLoginJsonExists; // issue #32

    static TRTHAPI* createInstance(const std::string& cryptoKey, const std::string& configDir);
    static TRTHAPI* getInstance();

    // prevent copy for singleton
    TRTHAPI(const TRTHAPI&) = delete;
    TRTHAPI& operator=(const TRTHAPI&) = delete;

    ~TRTHAPI();

    void start();
    void stop();
    // void join();
    void enqueueRequest(TRTHRequest req); // date format: YYYY-MM-DD

    void addUnavailableRequest(const TRTHRequest& req);
    size_t removeUnavailableRICs(std::vector<std::string>& originalRICs);

private:
    TRTHAPI(const std::string& cryptoKey, const std::string& configDir);

    void processRequests();

    int downloadAsCSV(const std::string& symbol, const std::string& requestDate); // date format: YYYY-MM-DD

    static TRTHAPI* s_pInst;

    const std::string m_key;
    const std::string m_cfgDir;

    std::queue<TRTHRequest> m_requests; ///> Queue for storing received TRTH downloading requests.
    std::vector<TRTHRequest> m_requestsUnavailable; ///> Memorizes unavailable/unrecognizable symbols/RICs.

    std::unique_ptr<std::thread> m_reqProcessorPtr; ///> Pointer to the unique Requests Processor.
    std::promise<void> m_reqProcQuitFlag; ///> To terminate the Requests Processor.

    std::mutex m_mtxReqs; ///> One per target; for guarding requests queue.
    std::condition_variable m_cvReqs; ///> For events of requests queue.

    std::mutex m_mtxReqsUnavail; ///> One per target; for guarding list of unavailable/unrecognizable requests.
};
