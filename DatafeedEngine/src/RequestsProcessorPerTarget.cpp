#include "RequestsProcessorPerTarget.h"

#include "FIXAcceptor.h"
#include "PSQL.h"
#include "TRTHAPI.h"

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/terminal/Common.h>

#ifdef DEBUG
#define __PRE_CONDITION__(ASSERT_EXPR) assert(ASSERT_EXPR)
#else
#define __PRE_CONDITION__(ASSERT_EXPR)
#endif

/**@brief Constructs a Requests Processor for one unique target.
 * @param targetID: The ID that uniquely identifies the target sending the requests.
 */
RequestsProcessorPerTarget::RequestsProcessorPerTarget(const std::string& targetID)
    : c_targetID(targetID)
{
    m_proc = std::thread(&RequestsProcessorPerTarget::processRequests, this); // run only after all other members were already initialized
}

/**@brief Destructs a Requests Processor for this target.
 *        It will firstly terminates the running processing thread.
 */
RequestsProcessorPerTarget::~RequestsProcessorPerTarget()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_procQuitFlag, m_cvQueues, m_proc);
}

/**
 * @brief Enqueue one Market Data request to process
 */
void RequestsProcessorPerTarget::enqueueMarketDataRequest(std::string reqID, std::vector<std::string>&& symbols, boost::posix_time::ptime&& startTime, boost::posix_time::ptime&& endTime)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxRequest);
        m_queueMarketReqs.emplace(std::move(reqID), std::move(symbols), std::move(startTime), std::move(endTime));
        m_queueReqTypes.push(REQUEST_TYPE::MARKET_DATA);
    }
    m_cvQueues.notify_one();
}

/**@brief Enqueue one Next Data request to process */
void RequestsProcessorPerTarget::enqueueNextDataRequest()
{
    {
        std::lock_guard<std::mutex> guard(m_mtxRequest);
        m_queueReqTypes.push(REQUEST_TYPE::NEXT_DATA);
    }
    m_cvQueues.notify_one();
}

/**@brief The unique Requests Processor thread for this target */
void RequestsProcessorPerTarget::processRequests()
{
    thread_local auto futQuit = m_procQuitFlag.get_future();
    std::future<bool> futLastMarketReq; // Updated by MARKET_DATA proc, consumed by subsequent NEXT_DATA proc

    while (true) {
        std::unique_lock<std::mutex> oneReqLock(m_mtxRequest);
        if (shift::concurrency::quitOrContinueConsumerThread(futQuit, m_cvQueues, oneReqLock, [this] { return !m_queueReqTypes.empty(); }))
            return;

        auto rt = m_queueReqTypes.front();
        m_queueReqTypes.pop();

        // dispatches and processes:
        switch (rt) {
        case REQUEST_TYPE::MARKET_DATA: {
            m_lastProcessedMarketReq = std::move(m_queueMarketReqs.front());
            m_queueMarketReqs.pop();
            futLastMarketReq = s_processRequestMarketData(oneReqLock, m_lastProcessedMarketReq, c_targetID);
        } break;
        case REQUEST_TYPE::NEXT_DATA: {
            s_processRequestNextData(oneReqLock, &futLastMarketReq, &m_lastProcessedMarketReq, c_targetID); // '&' indicates modification
        } break;
        } // switch
    } // while
}

/**@brief Processor helper for one Market Data request.
 * @param oneReqLock: The caller side lock that must be pre-locked before calling this.
 * @param marketReq: The Market Data request with necessary informations.
 * @param targetID: ID of this target.
 * @return future<bool> for waiting the TRTH downloads finish; bool value indicates whether it was a smooth, successful processing.
 */
/*static*/ std::future<bool> RequestsProcessorPerTarget::s_processRequestMarketData(const std::unique_lock<decltype(m_mtxRequest)>& oneReqLock, const MarketDataRequest& marketReq, const std::string& targetID)
{
    __PRE_CONDITION__(oneReqLock.owns_lock()); // This helper function is expected to work reliably only under the locked context of processRequests() !
    if (!oneReqLock.owns_lock())
        return {};

    auto& db = PSQLManager::getInstance();
    if (!db.isConnected()) {
        cerr << "ERROR: s_processRequestMarketData connect to DB failed." << endl;
        return {};
    }

    std::string tableName;
    const auto& symbols = marketReq.getSymbols();
    std::vector<std::promise<bool>> proms(symbols.size()); /* NOTE: Do NOT add/remove elements, to prevent any vector's internal reallocation.
                                                                Otherwise, element's memory location might be unreliable and hence communication
                                                                with TRTHAPI via promise<>* might NOT work! */
    std::vector<size_t> requestedSymbolsIndexes; // To postpone the TRTH requests to a later standalone loop so that the messages of DB query results and of TRTH downloads will not interleave in the terminal

    for (size_t idx{}; idx < symbols.size(); idx++) {
        auto flag = db.checkTableOfTradeAndQuoteRecordsExist(symbols[idx], marketReq.getDate(), tableName);
        using PTS = shift::database::TABLE_STATUS;

        bool isPerfect = true;

        switch (flag) {
        case PTS::EXISTS: {
            cout << "\033[0;32m" << std::setw(10 + 9) << std::right << tableName << " already exists." NO_COLOR << endl;
            proms[idx].set_value(isPerfect);
        } break;

        case PTS::NOT_EXIST:
            if (!TRTHAPI::s_bTRTHLoginJsonExists) {
                // Issue #32: Do NOT download if NO trthLogin.json exist on this computer
                proms[idx].set_value(true);
                TRTHAPI::getInstance()->addUnavailableRequest({ symbols[idx], marketReq.getDate(), &proms[idx] });
                break; // skip this symbol
            }
        case PTS::DB_ERROR:
        case PTS::OTHER_ERROR:
        default: {
            if (PTS::DB_ERROR == flag || PTS::OTHER_ERROR == flag) {
                isPerfect = false;
                cout << COLOR_WARNING "WARNING: " << (PTS::DB_ERROR == flag ? "" : "Other ") << "DB error was detected when querying table of [ " << symbols[idx] << " ] (process_request_market_data)..." NO_COLOR << endl;
            } else if (PTS::NOT_EXIST != flag) {
                cout << COLOR_WARNING "WARNING: Unknown database status was detected..." NO_COLOR << endl;
            }

            requestedSymbolsIndexes.push_back(idx);
            continue; // for
        }
        } // switch
    } // for

    for (auto idx : requestedSymbolsIndexes) {
        cout << "\033[0;33m" << std::setw(10 + 9) << std::right << symbols[idx] << " is absent." NO_COLOR << endl;
    }

    for (auto idx : requestedSymbolsIndexes) {
        // Here relies on the stability of the memory location that proms[idx] locates at!
        TRTHRequest trthReq{ symbols[idx], marketReq.getDate(), &proms[idx] };
        TRTHAPI::getInstance()->enqueueRequest(std::move(trthReq));
    }

    /** Creates and returns a future so as to
     * (1) await asynchronous TRTH downloads;
     * (2) provide chances for other enqueuing threads to proceed.
    */
    return std::async(
        std::launch::async // to run immediately
        ,
        [](std::vector<std::promise<bool>> proms /* owns promises by transfering/moving */
            ,
            std::string targetID, std::string requestID) {
            bool isPerfect = true;

            for (auto& prom : proms)
                isPerfect &= prom.get_future().get();

            s_announceDataReady(targetID, requestID);

            return isPerfect;
        } // []{}

        ,
        std::move(proms), targetID, marketReq.getRequestID()
        /* All these arguments will be used to create corresponding value(non-reference) objects for std::async's internal use.
                    If the argument is std::move-ed, then such object is move-constructed, otherwise copy-constructed.
                    After these, those objects will again move-constructs the lambda's parameters.
                    Especially, proms shall be moved because we need to reliably transfer the vector's internal resource
                    (i.e. array of promise<bool>), which is being shared by TRTHAPI part, into the new thread so that
                    the thread can communicate with TRTHAPI via promise.
                */
    ); // return
}

/**@brief Announces requested data is ready to send. */
/*static*/ void RequestsProcessorPerTarget::s_announceDataReady(const std::string& targetID, const std::string& requestID)
{
    static std::mutex mtxReadyMsg; // Its only purpose is to ensure atomically print message and send signal at the same time. Its lifetime will last from first time the program encountered it, to when the entire DE program terminates.
    std::lock_guard<std::mutex> guard(mtxReadyMsg);
    cout << '\n'
         << COLOR_PROMPT "Ready to send chunk to " << targetID << NO_COLOR << '\n'
         << endl;
    FIXAcceptor::sendNotice(targetID, requestID, "READY");
}

/**@brief Processor helper for one Next Data request.
 * @param oneReqLock: The caller side lock that must be pre-locked before calling this.
 * @param lastDownloadFutPtr: The pointer to the future that waiting for the last downloading. The future object will be mutated.
 * @param lastMarketRequestPtr: The pointer to last Market Data request with necessary informations. The request object will be mutated.
 * @param targetID: ID of this target.
*/
/*static*/ void RequestsProcessorPerTarget::s_processRequestNextData(const std::unique_lock<decltype(m_mtxRequest)>& oneReqLock, std::future<bool>* const lastDownloadFutPtr, MarketDataRequest* const lastMarketRequestPtr, const std::string& targetID)
{
    __PRE_CONDITION__(oneReqLock.owns_lock()); // This helper function is expected to work reliably only under the locked context of processRequests() !
    if (!oneReqLock.owns_lock())
        return;

    bool warnUnavailSkipped = false;

    if (lastDownloadFutPtr->valid()) {
        warnUnavailSkipped = true; // only need show warning for first Next Data since last Market Data
        bool isPerfect = lastDownloadFutPtr->get();
        (void)isPerfect; // just to avoid 'unused' warning
    }

    auto symbols = lastMarketRequestPtr->getSymbols();
    auto remCnt = TRTHAPI::getInstance()->removeUnavailableRICs(symbols);
    if (warnUnavailSkipped && remCnt) {
        cout << COLOR_WARNING "NOTE: " << remCnt << " unavailable symbols will not be sent.\n" NO_COLOR << endl;
    }

    // get start time and end time
    const boost::posix_time::ptime& sendFrom = lastMarketRequestPtr->getStartTime();
    boost::posix_time::ptime sendTo = sendFrom + boost::posix_time::minutes(15);
    if (sendTo >= lastMarketRequestPtr->getEndTime()) {
        sendTo = lastMarketRequestPtr->getEndTime();
    }

    // get raw data from database
    cout << "Sending data chunk " << COLOR "{ " << boost::posix_time::to_iso_extended_string(sendFrom) << " -- " << boost::posix_time::to_iso_extended_string(sendTo) << " }" NO_COLOR " to " << COLOR << targetID << NO_COLOR << " :\n"
         << endl;

    auto& db = PSQLManager::getInstance();
    if (!db.isConnected()) {
        cerr << "ERROR: s_processRequestNextData connect to DB failed." << endl;
        return;
    }

    size_t cnt = 0;
    for (const auto& symbol : symbols) {
        if (db.readSendRawData(targetID, symbol, sendFrom, sendTo))
            cout << '(' << ++cnt << '/' << symbols.size() << ")\n";
    }

    FIXAcceptor::sendNotice(targetID, lastMarketRequestPtr->getRequestID(), "SENDFINISH");

    lastMarketRequestPtr->updateTimeStart(sendTo);

    cout << "\nSend done.\n"
         << endl;
}

#undef __PRE_CONDITION__
