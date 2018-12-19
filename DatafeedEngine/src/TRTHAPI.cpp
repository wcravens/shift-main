#include "TRTHAPI.h"

#include "PSQL.h"

#include <cpprest/containerstream.h> // Async streams backed by STL containers
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h> // HTTP server
#include <cpprest/interopstream.h> // Bridges for integrating Async streams with STL and WinRT streams
#include <cpprest/json.h> // JSON library
#include <cpprest/producerconsumerstream.h> // Async streams for producer consumer scenarios
#include <cpprest/rawptrstream.h> // Async streams backed by raw pointer to memory
#include <cpprest/uri.h> // URI library
#include <cpprest/ws_client.h> // WebSocket client

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Functions.h>

#include <zlib.h>

using namespace std::chrono_literals;

#define CSTR_TRTHLOGIN_JSN \
    "trthLogin.json"
#define CSTR_EXTRACTRAW_JSN \
    "extract_raw.json"

/**@brief Formulate a CSV file name */
static inline std::string createCSVName(const std::string& symbol, const std::string& date /*yyyy-mm-dd*/)
{
    return symbol + date + ".csv";
}

static inline void addUnavailableRequest(std::mutex& unavailMtx, std::vector<TRTHRequest>& unavailReqs, const TRTHRequest& req)
{
    std::lock_guard<std::mutex> guard(unavailMtx);
    unavailReqs.push_back(req);
}

/**@brief Unzips a .GZ file (source) as a new file (destination) */
static bool unzip(const char* src, const char* dst, int* errn)
{
    unsigned char buf[GZ_BUF_LEN];
    gzFile in = gzopen(src, "rb");
    std::FILE* out = std::fopen(dst, "w");

    while (true) {
        int len = gzread(in, buf, GZ_BUF_LEN);
        if (len <= 0) {
            if (len < 0)
                gzerror(in, errn);
            break;
        }

        std::fwrite(buf, sizeof(Bytef), (size_t)len, out);
    }

    return !(std::fclose(out) || gzclose(in) != Z_OK || *errn < 0);
}

template <typename _Sink>
static auto operator>>(utility::ifstream_t&& istrm, _Sink&& s) -> decltype(istrm >> s)
{
    return istrm >> s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ TRTHAPI* TRTHAPI::s_pInst = nullptr;

TRTHAPI::TRTHAPI(const std::string& cryptoKey, const std::string& configDir)
    : m_key(cryptoKey)
    , m_cfgDir(configDir)
{
}

TRTHAPI::~TRTHAPI()
{
    stop();
    s_pInst = nullptr;
}

TRTHAPI* TRTHAPI::getInstance()
{
    return s_pInst;
}

TRTHAPI* TRTHAPI::createInstance(const std::string& cryptoKey, const std::string& configDir)
{
    static TRTHAPI s_inst(cryptoKey, configDir);
    s_pInst = &s_inst;
    return s_pInst;
}

/**@brief The unique Requests Processor thread for all requests */
void TRTHAPI::processRequests()
{
    auto futQuit = m_reqProcQuitFlag.get_future();
    auto& db = PSQLManager::getInstance();
    std::string tableName;

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxReqs);
        if (shift::concurrency::quitOrContinueConsumerThread(futQuit, m_cvReqs, lock, [this] { return !m_requests.empty(); }))
            return; // stopping TRTH was requested, shall terminate current thread

        TRTHRequest req;
        req = std::move(m_requests.front());
        m_requests.pop();
        lock.unlock(); // so that request supplier threads can push queue in time

        while (!db.isConnected()) {
            cout << "Request processor is trying to connect DB..." << endl;
            std::this_thread::sleep_for(5s);
            db.connectDB();
        }

        // Shall detect duplicated requests and skip them:
        auto dbFlag = db.checkTableOfTradeAndQuoteRecordsExist(req.symbol, req.date, tableName);
        using PMTS = PSQLManager::TABLE_STATUS;

        bool isPerfect = true;

        switch (dbFlag) {
        case PMTS::NOT_EXIST:
            break; // go to download from TRTH

        case PMTS::DB_ERROR:
        case PMTS::OTHER_ERROR: {
            isPerfect = false;
            cout << COLOR_ERROR "ERROR: Database was abnormal @ TRTHAPI::processRequests when querying symbol [ " << req.symbol << " ]. The symbol will be ignored!" NO_COLOR << endl;
            addUnavailableRequest(m_mtxReqsUnavail, m_requestsUnavailable, req);
        }
        case PMTS::EXISTS: {
            req.prom->set_value(isPerfect);
        }
            continue; // while(true)

        default:
            cout << COLOR_WARNING "Unknown database status!" NO_COLOR << endl;
            break;
        } // switch

        const auto flag = retrieveData(req.symbol, req.date);

        if (flag == 0) {
            std::string csvName = ::createCSVName(req.symbol, req.date);
            isPerfect &= db.isConnected() && db.saveCSVIntoDB(csvName, req.symbol, req.date);

            if (isPerfect) {
                std::remove(csvName.c_str());
                // cout << csvName << " deleted." << endl;

                std::lock_guard<std::mutex> guard(m_mtxReqsUnavail);
                auto newEnd = std::remove_if(m_requestsUnavailable.begin(), m_requestsUnavailable.end(), [&req](const TRTHRequest& elem) { return elem.symbol == req.symbol && elem.date == req.date; });
                m_requestsUnavailable.erase(newEnd, m_requestsUnavailable.end());
            }
        } else if (flag == 1) { //soap_status(RETRIEVE_STATUS::ERR_NOT_IN_TR)) {
            isPerfect = false;
            addUnavailableRequest(m_mtxReqsUnavail, m_requestsUnavailable, req);
        } else { // other RETRIEVE_STATUS
            isPerfect = false;
            cout << COLOR_ERROR "ERROR: TRTH cannot download [ " << req.symbol << " ]. The symbol will be skipped." << endl;
            // SoapStatusCodeInterpreter::interpret(flag);
            cout << NO_COLOR;
            addUnavailableRequest(m_mtxReqsUnavail, m_requestsUnavailable, req);
        }

        req.prom->set_value(isPerfect);
    } // while
}

void TRTHAPI::pushRequest(TRTHRequest req)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxReqs);
        m_requests.push(req);
    }
    m_cvReqs.notify_one();
}

/**@brief Creates and runs a Requests Processor thread */
void TRTHAPI::start()
{
    if (!m_reqProcessorPtr) {
        m_reqProcessorPtr.reset(new std::thread(&TRTHAPI::processRequests, this));
    }
}

/**@brief Terminates the Requests Processor. */
void TRTHAPI::stop()
{
    if (!m_reqProcessorPtr)
        return;

    cout << '\n'
         << COLOR "TRTH is stopping..." NO_COLOR << '\n'
         << flush;

    shift::concurrency::notifyConsumerThreadToQuit(m_reqProcQuitFlag, m_cvReqs, *m_reqProcessorPtr);
    m_reqProcessorPtr = nullptr;
}

/**@brief The main method to search, check, request and download data from TRTH. Gives processing status as feedback. */
int TRTHAPI::retrieveData(const std::string& symbol, const std::string& requestDate) // Date format: YYYY-MM-DD
{
    // prepare symbol format for TRTH request (_ to .)
    std::string ric = symbol;
    std::replace(ric.begin(), ric.end(), '_', '.');

    // HTTP client configuration
    web::http::client::http_client_config hcconf;
    hcconf.set_timeout(10min);

    web::json::value jCred;
    utility::ifstream_t{ m_cfgDir + CSTR_TRTHLOGIN_JSN } >> shift::crypto::Decryptor{ m_key } >> jCred;

    // Base http client for upcomming request
    web::http::client::http_client client("https://hosted.datascopeapi.reuters.com/RestApi/v1", hcconf);

    // cout << "Requesting Token..." << endl;

    web::http::http_request req;
    req.set_method(web::http::methods::POST);
    req.set_request_uri("/Authentication/RequestToken");
    req.headers().add(web::http::header_names::content_type, U("application/json; charset=utf-8"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));
    req.set_body(jCred);

    auto jAuth = client.request(req).get().extract_json().get();
    // cout << "Display json contents:" << endl;
    // jAuth.serialize(cout);
    // cout<<endl;

    const auto c_credToken = jAuth["value"].as_string();
    //cout << "\n\nAuth Token:\n" << c_credToken << endl;

    // cout << "Extract extract_raw.json..." << endl;
    web::json::value jExtr;
    utility::ifstream_t{ m_cfgDir + CSTR_EXTRACTRAW_JSN } >> jExtr;

    const std::string startTime = "00:00:00.000000000Z";
    const std::string endTime = "23:59:59.999999999Z";

    // Set appropriate parameters:
    jExtr["ExtractionRequest"]["Condition"]["QueryStartDate"] = web::json::value(utility::string_t(requestDate + 'T' + startTime));
    jExtr["ExtractionRequest"]["Condition"]["QueryEndDate"] = web::json::value(utility::string_t(requestDate + 'T' + endTime));

    web::json::value jRIC;
    jRIC["Identifier"] = web::json::value(utility::string_t(ric));
    jRIC["IdentifierType"] = web::json::value(utility::string_t("Ric"));
    std::vector<web::json::value> vRICs;
    vRICs.push_back(jRIC);
    jExtr["ExtractionRequest"]["IdentifierList"]["InstrumentIdentifiers"] = web::json::value::array(vRICs);

    // cout << "##########################################################" << endl;
    // jExtr.serialize(cout);
    // cout << "\n##########################################################" << endl;

    cout << endl
         << "Requesting " << COLOR << '[' << symbol << ']' << NO_COLOR << flush;

    req.set_method(web::http::methods::POST);
    req.set_request_uri("/Extractions/ExtractRaw");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + c_credToken);
    req.headers().add(web::http::header_names::content_type, U("application/json; odata=minimalmetadata")); // [[1]]
    //req.headers().add(U("Prefer"), U("respond-async"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));
    req.set_body(jExtr);

    auto extrJobID = utility::string_t{};
    try {
        web::http::http_response extrReqResp;
        shift::terminal::dots_awaiter([&extrReqResp, &client, &req] { extrReqResp = client.request(req).get(); });
        cout << endl;
        // cout << std::setw(20) << std::right << "Status:  " << extrReqResp.status_code() << endl;
        // cout << std::setw(20) << std::right << "Reason:  " << extrReqResp.reason_phrase() << endl;
        if (extrReqResp.status_code() >= 400)
            throw web::http::http_exception(COLOR_ERROR "ERROR: There was connection problem for ExtractRaw!" NO_COLOR);

        // utility::ofstream_t outf("./extracted.html");
        auto jExtrReqRes = extrReqResp.extract_json().get();
        // jExtrReqRes.serialize(outf);
        extrJobID = jExtrReqRes["JobId"].as_string();
        // cout << std::setw(20) << std::right << "JobID:  " << extrJobID << endl;
    } catch (const web::http::http_exception& e) {
        cout << "MSG=" << e.what() << endl;
        cout << "CODE=" << e.error_code() << endl;
    }

    if (extrJobID.empty())
        throw web::http::http_exception(COLOR_ERROR "ERROR: Cannot get RawExtractionResults:JobId !" NO_COLOR);

    cout << "Downloading" << flush;

    req.set_method(web::http::methods::GET);
    req.set_request_uri("/Extractions/RawExtractionResults('" + extrJobID + "')/$value");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + c_credToken);
    req.headers().add(web::http::header_names::content_type, U("application/javascript"));
    req.headers().add(web::http::header_names::accept_encoding, U("gzip"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));

    auto gzipStrm = client.request(req).get().body();
    if (!gzipStrm.is_valid())
        throw web::http::http_exception(COLOR_ERROR "ERROR: Cannot retrieve(GET) raw data file!" NO_COLOR);

    auto csvName = ::createCSVName(symbol, requestDate);
    auto gzipName = csvName + ".gz";

    Concurrency::streams::fstream::open_ostream(gzipName, std::ios::binary)
        .then([&gzipStrm](Concurrency::streams::ostream os) {
            size_t nread{};
            shift::terminal::dots_awaiter([&nread, &gzipStrm, &os] { nread = gzipStrm.read_to_end(os.streambuf()).get(); });
            cout << " - Size: " << nread << endl;
            os.flush();
        })
        .get();

    // cout << "Unzipping..." << endl;

    int errn = Z_OK;
    bool bRes = ::unzip(gzipName.c_str(), csvName.c_str(), &errn);
    (void)bRes; // no warning
    // cout << std::setw(20) << std::right << "Status:  " << (bRes ? "OK" : "ERR=");
    // if (!bRes)
    //     cout << errn;
    // cout << endl;

    std::remove(gzipName.c_str());

    return 0;
}

/**
 * @brief Removes unrecognizable symbols/RICs from the given symbols collection, with respect to the current TRTH contents.
 *        Note that, if any TRTH request detects a formerly unrecognized symbol from the remote now, at which time any ME
 *        concurrently requesting Next Data that contains this symbol, DE will either send or do not send this symbol to ME.
 *        Such behavior is caused by the concurrency contention characteristics.
 * @return Number of removed RICs.
 */
size_t TRTHAPI::removeUnavailableRICs(std::vector<std::string>& symbols)
{
    std::lock_guard<std::mutex> guard(m_mtxReqsUnavail);

    if (m_requestsUnavailable.empty() || symbols.empty())
        return 0;

    std::sort(symbols.begin(), symbols.end());
    std::stable_sort(m_requestsUnavailable.begin(), m_requestsUnavailable.end(), [](const TRTHRequest& lhs, const TRTHRequest& rhs) { return lhs.symbol < rhs.symbol; });

    auto oldSize = symbols.size();
    auto lastSearchPosInUnavail = m_requestsUnavailable.cbegin();
    auto pred = [this, &lastSearchPosInUnavail](const std::string& symbol) {
        lastSearchPosInUnavail = std::lower_bound(lastSearchPosInUnavail, m_requestsUnavailable.cend(), symbol, [](const TRTHRequest& elem, const std::string& symbol) { return elem.symbol < symbol; });

        if (m_requestsUnavailable.cend() == lastSearchPosInUnavail)
            return false;

        return symbol == lastSearchPosInUnavail->symbol;
    };

    auto newEnd = std::remove_if(symbols.begin(), symbols.end(), pred);
    symbols.erase(newEnd, symbols.end());

    return oldSize - symbols.size();
}

// /**@brief Prints friendlier SOAP error messages for specified funcion. */
// void TRTHAPI::printErrorDetails(std::string func)
// {
//     cout << COLOR_ERROR "gSOAP error";
//     // SoapStatusCodeInterpreter::interpret(m_apiProxy.error, "");
//     cout << "while calling function '" << func << "'." << endl;
//     if (m_apiProxy.soap_fault_string() != nullptr)
//         cout << "\nSOAP_fault_string:\n"
//                   << m_apiProxy.soap_fault_string() << endl;
//     if (m_apiProxy.soap_fault_detail() != nullptr)
//         cout << "\nSOAP_fault_detail:\n"
//                   << m_apiProxy.soap_fault_detail() << endl;

//     cout << COLOR;
//     // Provide hints for some common cases
//     switch (m_apiProxy.error) {
//     case SOAP_SSL_ERROR:
//         cout << "\nHint:\n"
//                      "This is an SSL error. Make sure stdsoap2.cpp is compiled with the command-line\n"
//                      "option -DWITH_OPENSSL and the environment variable SSL_CERT_FILE points to a\n"
//                      "Certification Authorities file such as the cacerts.pem that is included in the\n"
//                      "gSOAP installation."
//                   << endl;
//     break;
//     case SOAP_TCP_ERROR:
//         cout << "\nHint:\n"
//                      "This is a TCP error. Make sure you have assigned the correct HTTP proxy details\n"
//                      "in the application. If you are not using an HTTP proxy, make sure you have\n"
//                      "removed the proxy settings in the application source code."
//                   << endl;
//     break;
//     }
//     cout << NO_COLOR;
// }

// /**@brief Configure TRTH request parameters and submit the request */
// std::string TRTHAPI::submitRequest(ns2__Instrument* pInstrument, const std::string& date, ns2__TimeRange& timeRange)
// {
//     // Set up the fields and message types for the request. The names of these can be
//     // obtained by calling api.GetMessageTypes(), but we hard-code them here for brevity.

//     ns2__ArrayOfString fields1;
//     fields1.string.push_back("Price");
//     fields1.string.push_back("Volume");
//     fields1.string.push_back("Exchange Time");
//     fields1.string.push_back("Exchange ID");
//     ns2__MessageType timesale1;
//     timesale1.name = "Trade";
//     timesale1.fieldList = &fields1;

//     ns2__ArrayOfString fields;
//     fields.string.push_back("Bid Price");
//     fields.string.push_back("Bid Size");
//     fields.string.push_back("Ask Price");
//     fields.string.push_back("Ask Size");
//     fields.string.push_back("Quote Time");
//     fields.string.push_back("Quote Date");
//     fields.string.push_back("Buyer ID / Exchange ID");
//     fields.string.push_back("Seller ID / Exchange ID");

//     ns2__MessageType timesale;
//     timesale.name = "Quote";
//     timesale.fieldList = &fields;

//     ns2__ArrayOfMessageType messageTypes;
//     messageTypes.messageType.push_back(&timesale1);
//     messageTypes.messageType.push_back(&timesale);

//     ns2__RequestSpec request;
//     request.friendlyName = "ts Request";
//     request.date = date;
//     request.timeRange = &timeRange;
//     request.instrument = pInstrument;
//     request.requestType = ns2__RequestType__TimeAndSales;
//     request.messageTypeList = &messageTypes;
//     request.applyCorrections = false;
//     request.requestInGMT = false;
//     request.displayInGMT = false;
//     request.disableHeader = false;
//     request.dateFormat = ns2__RequestDateFormat__DDMMMYYYY;
//     request.disableDataPersistence = true;
//     request.includeCurrentRIC = false;
//     request.displayMicroseconds = true; // changed to 6 digits

//     _ns2__SubmitRequest submitRequest;
//     submitRequest.request = &request;

//     _ns2__SubmitRequestResponse submitRequestResponse;
//     auto statCode = soap_status{ m_apiProxy.SubmitRequest(&submitRequest, &submitRequestResponse) };
//     SoapStatusCodeInterpreter::interpret(statCode, statCode != SOAP_OK, "SOAP.APIProxy.SubmitRequest error");

//     return submitRequestResponse.requestID; // On error this will be an empty std::string.
// }

// /**@brief Asynchronously fetches the request result */
// ns2__RequestResult* TRTHAPI::getRequestResult(std::string& reqID)
// {
//     _ns2__GetRequestResult getRequestResult;
//     getRequestResult.requestID = reqID;

//     _ns2__GetRequestResultResponse response;

//     // Wait for the server side data processing to complete.
//     bool done = false;
//     while (!done) {
//         std::this_thread::sleep_for(2s); // wait between polls
//         int rc = m_apiProxy.GetRequestResult(&getRequestResult, &response);
//         SoapStatusCodeInterpreter::interpret(rc, rc != SOAP_OK, "SOAP.APIProxy.GetRequestResult error");
//         if (rc != 0
//             || response.result->status == ns2__RequestStatusCode__Complete
//             || response.result->status == ns2__RequestStatusCode__Aborted) {
//             done = true;
//         }
//     }

//     return response.result;
// }