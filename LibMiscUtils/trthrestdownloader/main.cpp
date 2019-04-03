/*
Compile:
    g++ -std=c++14 main.cpp -o TRTHRESTDownloader -lboost_system -lboost_program_options -lcrypto -lshift_miscutils -lssl -l:libcpprest.so.2.9 -lz

References:
    [1] "6 Requesting Time and Sales Data for RICs", from https://developers.thomsonreuters.com/sites/default/files/Thomson%20Reuters%20Tick%20History%2011.0%20REST%20API%20Use%20Cases%20Reference%20v1.0.pdf
*/

#include "crypto/Decryptor.h"
#include "terminal/Common.h"

#include <iomanip>

#include <boost/program_options.hpp>

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

#include <zlib.h>

using namespace std::chrono_literals;

#define GZ_BUF_LEN (1024 * 10)

#define CSTR_HELP \
    "help"
#define CSTR_JSONDIR \
    "jsondir"
#define CSTR_KEY \
    "key"
#define CSTR_OUTPUT \
    "output"
#define CSTR_UNZIP \
    "unzip"

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

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc("\nUSAGE: ./TRTHRESTDownloader [options] <args>\n\n\tThis is the TRTH Downloader utility application.\n\t"
                                 "If multiple RICs were requested, they are consecutively compacted in the single same CSV file.\n\t"
                                 "Please configure the 'cred.json'(encrypted) and 'extract_raw.json' files to customize your downloads.\n\nOPTIONS");
    desc.add_options() //
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_JSONDIR ",j", po::value<std::string>(), "shared directory of cred.json and extract_raw.json (default: ./)") //
        (CSTR_KEY ",k", po::value<std::string>(), "crypto key of cred.json") //
        (CSTR_OUTPUT ",o", po::value<std::string>(), "name of the output file without extension (default: raw_data)") //
        (CSTR_UNZIP ",u", "unzip the output") //
        ; // add_options

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const boost::program_options::error& e) {
        cerr << COLOR_ERROR "ERROR: " << e.what() << NO_COLOR << endl;
        return 1;
    }

    if (vm.count(CSTR_HELP)) {
        cout << desc << endl;
        return 0;
    }

    auto t1 = std::chrono::system_clock::now();

    cout << endl;

    std::string strJsonDir = "./";
    if (vm.count(CSTR_JSONDIR)) {
        strJsonDir = vm[CSTR_JSONDIR].as<std::string>();
        if (strJsonDir.back() != '/')
            strJsonDir.push_back('/');
    }

    std::string cryptoKey = "SHIFT123";
    if (vm.count(CSTR_KEY)) {
        cryptoKey = vm[CSTR_KEY].as<std::string>();
    } else {
        cout << COLOR "The built-in initial key 'SHIFT123' is used for reading encrypted json file." NO_COLOR << endl;
        cout << endl;
    }

    std::string strOutput = "raw_data";
    if (vm.count(CSTR_OUTPUT)) {
        strOutput = vm[CSTR_OUTPUT].as<std::string>();
    }

    bool doUnzip = false;
    if (vm.count(CSTR_UNZIP)) {
        doUnzip = true;
    }

    web::http::client::http_client_config hcconf;
    hcconf.set_timeout(10min);

    web::json::value cred;
    utility::ifstream_t infCred{ strJsonDir + "cred.json" };
    if (!infCred.good()) {
        cout << COLOR_ERROR "ERROR: "
             << "Cannot successfully access cred.json!" << NO_COLOR << endl;
        cout << desc << endl;
        return 2;
    }

    shift::crypto::Decryptor dec{ cryptoKey };
    infCred >> dec >> cred;
    infCred.close();

    // Create htturi_builderp_client to send the request.
    web::http::client::http_client client("https://hosted.datascopeapi.reuters.com/RestApi/v1", hcconf);

    cout << "Requesting Token..." << endl;

    web::http::http_request req;
    req.set_method(web::http::methods::POST);
    req.set_request_uri("/Authentication/RequestToken");
    req.headers().add(web::http::header_names::content_type, U("application/json; charset=utf-8"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));
    req.set_body(cred);

    auto reqTaskCred = client.request(req);

    auto jtask = reqTaskCred.get().extract_json();
    // cout << "############ Display json contents:" << endl;
    // jtask.get().serialize(cout);
    // cout << "\n############" << endl;

    const auto credToken = jtask.get()["value"].as_string();
    //cout << "\n\nAuth Token:\n" << credToken << endl;

    cout << endl;

    utility::ifstream_t infExtract(strJsonDir + "extract_raw.json");
    if (!infExtract.good()) {
        cout << COLOR_ERROR "ERROR: "
             << "Cannot successfully access extract_raw.json!" << NO_COLOR << endl;
        cout << desc << endl;
        return 3;
    }

    web::json::value cmdExtract;
    try {
        infExtract >> cmdExtract;
    } catch (const web::json::json_exception& e) {
        cerr << COLOR_ERROR "ERROR: "
             << "Please check and correct extract_raw.json" << NO_COLOR << endl;
        return 4;
    }
    // cout << "##########################################################" << endl;
    cmdExtract.serialize(cout);
    // cout << "\n##########################################################" << endl;

    cout << "Requesting Extraction..." << endl;

    req.set_method(web::http::methods::POST);
    req.set_request_uri("/Extractions/ExtractRaw");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + credToken);
    req.headers().add(web::http::header_names::content_type, U("application/json; odata=minimalmetadata"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));
    req.set_body(cmdExtract);

    auto extrRawJobID = utility::string_t("");
    try {
        client.request(req).then([&extrRawJobID](web::http::http_response resp) {
                               cout << std::setw(20) << std::right << "Status:  " << resp.to_string() << endl;
                               cout << std::setw(20) << std::right << "Reason:  " << resp.reason_phrase() << endl;

                               // utility::ofstream_t outf("extracted.html");
                               auto jval = resp.extract_json().get();
                               // jval.serialize(outf);

                               extrRawJobID = jval["JobId"].as_string();
                               cout << std::setw(20) << std::right << "JobID:  " << extrRawJobID << endl;
                           })
            .get();
    } catch (const web::http::http_exception& e) {
        cout << "MSG=" << e.what() << endl;
        cout << "CODE=" << e.error_code() << endl;
    }

    if (extrRawJobID.empty()) {
        // throw web::http::http_exception("Cannot get RawExtractionResults:JobId!");
        cerr << COLOR_ERROR "ERROR: "
             << "Cannot get RawExtractionResults:JobId!" << NO_COLOR << endl;
        return 5;
    }

    cout << endl;

    cout << "Requesting Results..." << endl;

    req.set_method(web::http::methods::GET);
    req.set_request_uri("/Extractions/RawExtractionResults('" + extrRawJobID + "')/$value");

    req.headers().clear();
    req.headers().add(web::http::header_names::authorization, U("Token ") + credToken);
    req.headers().add(web::http::header_names::content_type, U("application/javascript"));
    req.headers().add(web::http::header_names::accept_encoding, U("gzip"));
    req.headers().add(web::http::header_names::connection, U("keep-alive"));

    auto gzipStrm = client.request(req).get().body();
    if (!gzipStrm.is_valid()) {
        // throw web::http::http_exception("Cannot retrieve(GET) raw data file!");
        cerr << COLOR_ERROR "ERROR: "
             << "Cannot retrieve(GET) raw data file!" << NO_COLOR << endl;
        return 6;
    }

    Concurrency::streams::fstream::open_ostream(strOutput + ".gz", std::ios::binary).then([&](Concurrency::streams::ostream os) {
                                                                                        size_t nread = gzipStrm.read_to_end(os.streambuf()).get();
                                                                                        cout << std::setw(20) << std::right << "Size:  " << nread << " bytes" << endl;
                                                                                        os.flush();
                                                                                    })
        .get();

    cout << endl;

    if (doUnzip) {
        cout << "Unzipping..." << endl;
        int errn = Z_OK;
        bool bRes = unzip((strOutput + ".gz").c_str(), (strOutput + ".csv").c_str(), &errn);
        cout << std::setw(20) << std::right << "Status:  " << (bRes ? "OK" : "ERR=");
        if (!bRes)
            cout << errn;
        else
            std::remove((strOutput + ".gz").c_str());
        cout << endl;
        cout << endl;
    }

    auto t2 = std::chrono::system_clock::now();
    cout << "Process took " << std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count() << "s to run.\n"
         << endl;

    return 0;
}
