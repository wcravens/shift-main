#include "DBConnector.h"
#include "MainClient.h"
#include "SHIFTServiceHandler.h"

#if __has_include(<filesystem>)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <future>
#include <thread>

#include <pwd.h>

#include <boost/program_options.hpp>

#include <shift/coreclient/FIXInitiator.h>

#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Options.h>

using namespace std::chrono_literals;

/* PROGRAM OPTIONS */
#define CSTR_HELP \
    "help"
#define CSTR_CONFIG \
    "config"
#define CSTR_KEY \
    "key"
#define CSTR_DBLOGIN_TXT \
    "dbLogin.txt"
#define CSTR_TIMEOUT \
    "timeout"
#define CSTR_VERBOSE \
    "verbose"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

auto main(int argc, char** argv) -> int
{
    /**
     * @brief Centralizes and classifies all necessary parameters and
     * hides them behind one variable to ease understanding and debugging.
     */
    struct {
        std::string configDir;
        std::string cryptoKey;
        struct { // timeout settings
            using min_t = std::chrono::minutes::rep;
            bool isSet;
            min_t minutes;
        } timer;
        bool isVerbose;
    } params = {
        "/usr/local/share/shift/WebClient/", // default installation folder for configuration
        "SHIFT123", // built-in initial crypto key used for encrypting dbLogin.txt
        {
            false,
            0,
        },
        false,
    };

    po::options_description desc("\nUSAGE: ./WebClient [options] <args>\n\n\tThis is the client application.\n\tThe client connects with BrokerageCenter.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_KEY ",k", po::value<std::string>(), "key of " CSTR_DBLOGIN_TXT " file") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        ; // add_options

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const boost::program_options::error& e) {
        cerr << COLOR_ERROR "ERROR: " << e.what() << NO_COLOR << endl;
        return 1;
    } catch (...) {
        cerr << COLOR_ERROR "ERROR: Exception of unknown type!" NO_COLOR << endl;
        return 2;
    }

    if (vm.count(CSTR_HELP) > 0) {
        cout << '\n'
             << desc << '\n'
             << endl;
        return 0;
    }

    if (vm.count(CSTR_VERBOSE) > 0) {
        params.isVerbose = true;
    }
    voh_t voh(cout, params.isVerbose);

    if (vm.count(CSTR_CONFIG) > 0) {
        params.configDir = vm[CSTR_CONFIG].as<std::string>();
        cout << COLOR "'config' directory was set to "
             << params.configDir << ".\n" NO_COLOR << endl;
    } else {
        cout << COLOR "'config' directory was not set. Using default config dir: " << params.configDir << NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_KEY) > 0) {
        params.cryptoKey = vm[CSTR_KEY].as<std::string>();
        cout << COLOR "'crypto key' was set to "
             << params.cryptoKey << ".\n" NO_COLOR << endl;
    } else {
        cout << COLOR << "'crypto key' was not set. Using default key: " << params.cryptoKey << NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_TIMEOUT) > 0) {
        params.timer.minutes = vm[CSTR_TIMEOUT].as<decltype(params.timer)::min_t>();
        if (params.timer.minutes > 0) {
            params.timer.isSet = true;
        } else {
            cout << COLOR "Note: The timeout option is ignored because of the given value." NO_COLOR << '\n'
                 << endl;
        }
    }

    DBConnector::getInstance().init(params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    while (true) {
        if (!DBConnector::getInstance().connectDB()) {
            cout.clear();
            cout << COLOR_ERROR "DB ERROR: Failed to connect database." NO_COLOR << endl;
            cout << "\tRetry ('Y') connection to database ? : ";
            voh_t { cout, params.isVerbose, true };

            char cmd = cin.get();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
            if ('Y' != cmd && 'y' != cmd) {
                return 5;
            }
        } else {
            cout << "DB connection OK.\n"
                 << endl;
            break;
        }
    }

    MainClient mainClient { "webclient" };

    mainClient.setVerbose(params.isVerbose);
    try {
        shift::FIXInitiator::getInstance().connectBrokerageCenter(params.configDir + "initiator.cfg", &mainClient, "password", params.isVerbose);
    } catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << "Something went wrong: " << e.what() << std::endl;
        std::cout << std::endl;
        return 1;
    }

    // get all company names and send it to front - start
    mainClient.requestCompanyNames();
    std::thread tCompanyNames(&MainClient::sendCompanyNamesToFront, &mainClient);
    tCompanyNames.detach();
    // get all company names and send it to front - end

    std::thread tRecReq(&MainClient::receiveRequestFromPHP, &mainClient);

    mainClient.subAllOrderBook();
    mainClient.subAllCandlestickData();
    mainClient.sendStockListToFront();

    // start thread of WebClient::checkEverySecond function
    std::thread tCheck(&MainClient::checkEverySecond, &mainClient);

    // thrift service start
    int port = 9090;
    apache::thrift::stdcxx::shared_ptr<SHIFTServiceHandler> handler(new SHIFTServiceHandler());
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TProcessor> processor(new SHIFTServiceProcessor(handler));
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TServerTransport> serverTransport(new apache::thrift::transport::TServerSocket(port));
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TTransportFactory> transportFactory(new apache::thrift::transport::TBufferedTransportFactory());
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TProtocolFactory> protocolFactory(new apache::thrift::server::TBinaryProtocolFactory());

    apache::thrift::server::TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    std::thread tThrift(&apache::thrift::server::TSimpleServer::serve, &server);

    // create 'done' file in ~/.shift/WebClient to signalize shell that service is done loading
    // (directory is also created if it does not exist)
    const char* homeDir = nullptr;
    if ((homeDir = getenv("HOME")) == nullptr) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    std::string servicePath { homeDir };
    servicePath += "/.shift/WebClient";
#if __has_include(<filesystem>)
    std::filesystem::create_directories(servicePath);
#else
    std::experimental::filesystem::create_directories(servicePath);
#endif
    std::ofstream doneSignal { servicePath + "/done" };
    doneSignal.close();

    // running in background
    if (params.timer.isSet) {
        cout.clear();
        cout << '\n'
             << COLOR_PROMPT "Timer begins ( " << params.timer.minutes << " mins )..." NO_COLOR << '\n'
             << endl;

        voh_t { cout, params.isVerbose, true };
        std::this_thread::sleep_for(params.timer.minutes * 1min);
    } else {
        std::async(std::launch::async // no delay
            ,
            [&params] {
                while (true) {
                    cout.clear();
                    cout << '\n'
                         << COLOR_PROMPT "The WebClient is running. (Enter 'T' to stop)" NO_COLOR << '\n'
                         << endl;
                    voh_t { cout, params.isVerbose, true };

                    char cmd = cin.get(); // wait
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
                    if ('T' == cmd || 't' == cmd) {
                        return;
                    }
                }
            })
            .get(); // this_thread will wait for user terminating acceptor.
    }

    // close program
    server.stop();
    tThrift.join();

    MainClient::s_isTimeout = true;
    tCheck.join();
    tRecReq.join();

    shift::FIXInitiator::getInstance().disconnectBrokerageCenter();

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
        cout << endl;
    }

    return 0;
}
