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
private:
    static std::unique_ptr<TRTHAPI> s_pInst;
    static std::once_flag s_instFlag;

    // TRTHApiBindingProxy m_apiProxy; ///> A proxy used for accessing SOAP APIs
    // ns2__CredentialsHeader m_soapCredHeader;
    const std::string m_key;
    const std::string m_cfgDir;

    std::queue<TRTHRequest> m_requests; ///> Queue for storing received TRTH downloading requests
    std::vector<TRTHRequest> m_requestsUnavailable; ///> Memorizes unavailable/unrecognizable symbols/RICs

    std::unique_ptr<std::thread> m_reqProcessorPtr; ///> Pointer to the unique Requests Processor
    std::promise<void> m_reqProcQuitFlag; ///> To terminate the Requests Processor

    std::mutex m_mtxReqs; ///> One per target; for guarding requests queue
    std::condition_variable m_cvReqs; ///> For events of requests queue

    std::mutex m_mtxReqsUnavail; ///> One per target; for guarding list of unavailable/unrecognizable requests

    void processRequests();
    TRTHAPI(const std::string& usr, const std::string& pwd);

public:
    static TRTHAPI* createInstance(const std::string& key, const std::string& dir);
    static TRTHAPI* getInstance();

    // prevent copy for singleton
    TRTHAPI(const TRTHAPI&) = delete;
    TRTHAPI& operator=(const TRTHAPI&) = delete;

    ~TRTHAPI();

    void start();
    void stop();
    // void join();
    void pushRequest(TRTHRequest req); // Date format: YYYY-MM-DD

    size_t removeUnavailableRICs(std::vector<std::string>&);

private:
    // /**@brief The extension to soap status codes for TRTHAPI's internal use. */
    // enum class RETRIEVE_STATUS : soap_status
    // {
    //     ERR_NOT_IN_TR = 600, // first code that is user definable, see stdsoap2.h
    //     ERR_DATA_ACQ_INCOMPLETE,
    //     ERR_LOCAL_FILE,

    //     __LAST_STATUS__ // use this as the first value of other enum class (if any)
    // };

    int retrieveData(const std::string& symbol, const std::string& requestDate); // Date format: YYYY-MM-DD

    // void printErrorDetails(std::string);

    // ns2__Instrument* findExactMatch(ns2__ArrayOfInstrument*, const std::string&);

    // std::string submitRequest(ns2__Instrument*, const std::string&, ns2__TimeRange&);

    // ns2__RequestResult* getRequestResult(std::string&);
};
