#include "TRTHAPI.h"

#include "PSQL.h"

#include <zlib.h>

#include <cpprest/containerstream.h> // async streams backed by STL containers
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h> // HTTP server
#include <cpprest/interopstream.h> // bridges for integrating Async streams with STL and WinRT streams
#include <cpprest/json.h> // JSON library
#include <cpprest/producerconsumerstream.h> // async streams for producer consumer scenarios
#include <cpprest/rawptrstream.h> // async streams backed by raw pointer to memory
#include <cpprest/uri.h> // URI library
#include <cpprest/ws_client.h> // WebSocket client

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Functions.h>

using namespace std::chrono_literals;

#define CSTR_TRTHLOGIN_JSN \
    "trthLogin.json"
#define CSTR_EXTRACTRAW_JSN \
    "extractRaw.json"

/**
 * @brief Formulate a CSV file name.
 */
static inline auto s_createCSVName(const std::string& symbol, const std::string& date /*yyyy-mm-dd*/) -> std::string
{
    return symbol + date + ".csv";
}

/**
 * @brief Unzips a .GZ file (source) as a new file (destination).
 */
static auto unzip(const char* src, const char* dst, int* errn) -> bool
{
    unsigned char buf[GZ_BUF_LEN];
    gzFile in = gzopen(src, "rb");
    std::FILE* out = std::fopen(dst, "w");

    while (true) {
        int len = gzread(in, buf, GZ_BUF_LEN);
        if (len <= 0) {
            if (len < 0) {
                gzerror(in, errn);
            }
            break;
        }

        std::fwrite(buf, sizeof(Bytef), (size_t)len, out);
    }

    return !(std::fclose(out) != 0 || gzclose(in) != Z_OK || *errn < 0);
}

template <typename _Sink>
static auto operator>>(utility::ifstream_t&& istrm, _Sink&& s) -> decltype(istrm >> s)
{
    return istrm >> s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

/* static */ TRTHAPI* TRTHAPI::s_pInst = nullptr;
/* static */ std::atomic<bool> TRTHAPI::s_bTRTHLoginJsonExists { false };

TRTHAPI::TRTHAPI(std::string cryptoKey, std::string configDir)
    : m_key { std::move(cryptoKey) }
    , m_cfgDir { std::move(configDir) }
{
}

TRTHAPI::~TRTHAPI()
{
    stop();
    s_pInst = nullptr;
}

/* static */ auto TRTHAPI::createInstance(const std::string& cryptoKey, const std::string& configDir) -> TRTHAPI&
{
    static TRTHAPI s_inst(cryptoKey, configDir);
    s_pInst = &s_inst;
    return s_inst;
}

/* static */ auto TRTHAPI::getInstance() -> TRTHAPI&
{
    return *s_pInst;
}

/**
 * @brief Creates and runs a Requests Processor thread.
 */
void TRTHAPI::start()
{
    if (!m_reqProcessorPtr) {
        m_reqProcessorPtr = std::make_unique<std::thread>(&TRTHAPI::processRequests, this);
    }
}

/**
 * @brief Terminates the Requests Processor.
 */
void TRTHAPI::stop()
{
    if (!m_reqProcessorPtr) {
        return;
    }

    cout << '\n'
         << COLOR "TRTH is stopping..." NO_COLOR << '\n'
         << flush;

    shift::concurrency::notifyConsumerThreadToQuit(m_reqProcQuitFlag, m_cvReqs, *m_reqProcessorPtr);
    m_reqProcessorPtr = nullptr;
}

void TRTHAPI::enqueueRequest(TRTHRequest req)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxReqs);
        m_requests.push(req);
    }
    m_cvReqs.notify_one();
}

void TRTHAPI::addUnavailableRequest(const TRTHRequest& req)
{
    std::lock_guard<std::mutex> guard(m_mtxReqsUnavail);
    m_requestsUnavailable.push_back(req);
}

/**
 * @brief Removes unrecognizable symbols/RICs from the given symbols/RICs collection, with respect to the current TRTH contents.
 *        Note that, if any TRTH request detects a formerly unrecognized symbol from the remote now, at which time any ME
 *        concurrently requesting Next Data that contains this symbol, DE will either send or do not send this symbol to ME.
 *        Such behavior is caused by the concurrency contention characteristics.
 * @return Number of removed RICs.
 */
auto TRTHAPI::removeUnavailableRICs(std::vector<std::string>& originalRICs) -> size_t
{
    std::lock_guard<std::mutex> guard(m_mtxReqsUnavail);

    if (m_requestsUnavailable.empty() || originalRICs.empty()) {
        return 0;
    }

    std::sort(originalRICs.begin(), originalRICs.end());
    std::stable_sort(m_requestsUnavailable.begin(), m_requestsUnavailable.end(), [](const TRTHRequest& lhs, const TRTHRequest& rhs) { return lhs.symbol < rhs.symbol; });

    const auto oldSize = originalRICs.size();
    auto lastSearchPosInUnavail = m_requestsUnavailable.cbegin();

    auto pred = [this, &lastSearchPosInUnavail](const std::string& symbol) {
        lastSearchPosInUnavail = std::lower_bound(lastSearchPosInUnavail, m_requestsUnavailable.cend(), symbol, [](const TRTHRequest& elem, const std::string& symbol) { return elem.symbol < symbol; });

        if (m_requestsUnavailable.cend() == lastSearchPosInUnavail) {
            return false;
        }

        return symbol == lastSearchPosInUnavail->symbol;
    };

    originalRICs.erase(std::remove_if(originalRICs.begin(), originalRICs.end(), pred), originalRICs.end());

    return oldSize - originalRICs.size();
}

/**
 * @brief The unique Requests Processor thread for all requests.
 */
void TRTHAPI::processRequests()
{
    auto futQuit = m_reqProcQuitFlag.get_future();
    auto& db = PSQLManager::getInstance();
    std::string tableName;
    int flag = -1;

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxReqs);
        if (shift::concurrency::quitOrContinueConsumerThread(futQuit, m_cvReqs, lock, [this] { return !m_requests.empty(); })) {
            return; // stopping TRTH was requested, shall terminate current thread
        }

        TRTHRequest req;
        req = std::move(m_requests.front());
        m_requests.pop();
        lock.unlock(); // so that request supplier threads can push queue in time

        while (!db.isConnected()) {
            cout << "Request processor is trying to connect DB..." << endl;
            std::this_thread::sleep_for(5s);
            db.connectDB();
        }

        // shall detect duplicated requests and skip them:
        auto dbFlag = db.checkTableOfTradeAndQuoteRecordsExist(req.symbol, req.date, &tableName);
        using PTS = shift::database::TABLE_STATUS;

        bool isPerfect = true;

        switch (dbFlag) {
        case PTS::NOT_EXIST:
            if (s_bTRTHLoginJsonExists) {
                break; // go to download it from TRTH
            }

            // issue #32: DO NOT download if NO trthLogin.json exists on this computer
            req.prom->set_value(true);
            addUnavailableRequest(req);
            continue; // while(true)

        case PTS::DB_ERROR:
        case PTS::OTHER_ERROR: {
            isPerfect = false;
            cout << COLOR_ERROR "ERROR: Database was abnormal @ TRTHAPI::processRequests when querying symbol [ " << req.symbol << " ]. The symbol will be ignored!" NO_COLOR << endl;
            addUnavailableRequest(req);
        }
        case PTS::EXISTS: {
            req.prom->set_value(isPerfect);
            continue; // while(true)
        }

        default:
            cout << COLOR_WARNING "Unknown database status!" NO_COLOR << endl;
            break;
        } // switch

        try {
            flag = downloadAsCSV(req.symbol, req.date);
        } catch (const web::http::http_exception& e) {
            cout << e.what() << endl;
            flag = 2;
        }

        if (flag == 0) {
            std::string csvName = ::s_createCSVName(req.symbol, req.date);
            isPerfect &= db.isConnected() && db.saveCSVIntoDB(csvName, req.symbol, req.date);

            if (isPerfect) {
                std::remove(csvName.c_str());
                // cout << csvName << " deleted." << endl;

                // it's available in DB now, untrack it from unavailables
                std::lock_guard<std::mutex> guard(m_mtxReqsUnavail);
                auto newEnd = std::remove_if(m_requestsUnavailable.begin(), m_requestsUnavailable.end(), [&req](const TRTHRequest& elem) { return elem.symbol == req.symbol && elem.date == req.date; });
                m_requestsUnavailable.erase(newEnd, m_requestsUnavailable.end());
            }
        } else if (flag == 1) { // not in TR
            isPerfect = false;
            addUnavailableRequest(req);
        } else { // other RETRIEVE_STATUS
            isPerfect = false;
            cout << COLOR_ERROR "ERROR: TRTH cannot download [ " << req.symbol << " ]. The symbol will be skipped." << endl;
            cout << NO_COLOR;
            addUnavailableRequest(req);
        }

        req.prom->set_value(isPerfect);
    } // while
}

/**
 * @brief The main method to search, check, request and download data from TRTH. Gives processing status as feedback.
 */
auto TRTHAPI::downloadAsCSV(const std::string& symbol, const std::string& requestDate) -> int // date format: YYYY-MM-DD
{
    // prepare symbol format for TRTH request
    std::string ric = symbol;
    ::cvtRICToDEInternalRepresentation(&ric, true); // '_' => '.'

    // HTTP client configuration
    web::http::client::http_client_config hcconf;
    hcconf.set_timeout(15min);

    web::json::value jCred;
    utility::ifstream_t { m_cfgDir + CSTR_TRTHLOGIN_JSN } >> shift::crypto::Decryptor { m_key } >> jCred;

    // base HTTP client for upcomming request
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
    // cout << "\n\nAuth Token:\n" << c_credToken << endl;

    // cout << "Extract extractRaw.json..." << endl;
    web::json::value jExtr;
    utility::ifstream_t { m_cfgDir + CSTR_EXTRACTRAW_JSN } >> jExtr;

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
    // cout << "\n########################################################" << endl;

    cout << endl
         << "Requesting " << COLOR << '[' << symbol << ']' << NO_COLOR << flush;

    req.set_method(web::http::methods::POST);
    req.set_request_uri("/Extractions/ExtractRaw");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + c_credToken);
    req.headers().add(web::http::header_names::content_type, U("application/json; odata=minimalmetadata")); // [[1]]
    // req.headers().add(U("Prefer"), U("respond-async"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));
    req.set_body(jExtr);

    auto extrJobID = utility::string_t {};
    try {
        web::http::http_response extrReqResp;
        shift::terminal::dots_awaiter([&extrReqResp, &client, &req] { extrReqResp = client.request(req).get(); });
        cout << endl;
        // cout << std::setw(20) << std::right << "Status:  " << extrReqResp.status_code() << endl;
        // cout << std::setw(20) << std::right << "Reason:  " << extrReqResp.reason_phrase() << endl;
        if (extrReqResp.status_code() >= 400) {
            throw web::http::http_exception(COLOR_ERROR "ERROR: There was connection problem for ExtractRaw!" NO_COLOR);
        }

        // utility::ofstream_t outf("./extracted.html");
        auto jExtrReqRes = extrReqResp.extract_json().get();
        // jExtrReqRes.serialize(outf);
        extrJobID = jExtrReqRes["JobId"].as_string();
        // cout << std::setw(20) << std::right << "JobID:  " << extrJobID << endl;
    } catch (const web::http::http_exception& e) {
        cout << "MSG=" << e.what() << endl;
        cout << "CODE=" << e.error_code() << endl;
    }

    if (extrJobID.empty()) {
        throw web::http::http_exception(COLOR_ERROR "ERROR: Cannot get RawExtractionResults:JobId!" NO_COLOR);
    }

    cout << "Downloading" << flush;

    req.set_method(web::http::methods::GET);
    req.set_request_uri("/Extractions/RawExtractionResults('" + extrJobID + "')/$value");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + c_credToken);
    req.headers().add(web::http::header_names::content_type, U("application/javascript"));
    req.headers().add(web::http::header_names::accept_encoding, U("gzip"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));

    auto gzipStrm = client.request(req).get().body();
    if (!gzipStrm.is_valid()) {
        throw web::http::http_exception(COLOR_ERROR "ERROR: Cannot retrieve(GET) raw data file!" NO_COLOR);
    }

    auto csvName = ::s_createCSVName(symbol, requestDate);
    auto gzipName = csvName + ".gz";

    Concurrency::streams::fstream::open_ostream(gzipName, std::ios::binary)
        .then([&gzipStrm](Concurrency::streams::ostream os) {
            size_t nread {};
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

    if (std::ifstream { csvName }.peek() == std::ifstream::traits_type::eof()) { // empty CSV file ?
        cout << COLOR_WARNING "WARNING: No data for this RIC!" << NO_COLOR << endl;
        std::remove(csvName.c_str());
        return 1; // e.g. RIC does not exist in TRTH
    }

    return 0;
}
