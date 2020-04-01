#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Parameters.h"
#include "StockMarket.h"
#include "TimeSetting.h"
#include "configFunctions.h"

#include <atomic>
#if __has_include(<filesystem>)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <future>

#include <pwd.h>

#include <boost/program_options.hpp>

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
#define CSTR_DATE \
    "date"
#define CSTR_STARTTIME \
    "starttime"
#define CSTR_ENDTIME \
    "endtime"
#define CSTR_TIMEOUT \
    "timeout"
#define CSTR_VERBOSE \
    "verbose"
#define CSTR_MANUAL \
    "manual"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

static std::atomic<bool> s_isRequestingData { true };

/*
 * @brief Function to request data chunks in the background.
 */
static void s_requestDatafeedEngineData(std::string configFile, bool verbose, std::string cryptoKey, std::string dbConfigFile, std::string requestID, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime, std::vector<std::string> symbols, int numSecondsPerDataChunk, int experimentSpeed)
{
    if (!FIXInitiator::getInstance()->connectDatafeedEngine(configFile, verbose, cryptoKey, dbConfigFile)) {
        // cout << "DEBUG: s_requestDatafeedEngineData cannot connect DE!" << endl;
        return;
    }

    // send request to Datafeed Engine for TRTH data and *wait* until data is ready
    if (FIXInitiator::getInstance()->sendSecurityListRequestAwait(requestID, startTime, endTime, symbols, numSecondsPerDataChunk)) {
        auto requestOnce = [](auto* pStartTime) {
            FIXInitiator::getInstance()->sendNextDataRequest();
            *pStartTime += boost::posix_time::seconds(::DURATION_PER_DATA_CHUNK.count());
        };

        // since real time (i.e. absolute time) may have been elapsed considerably because of the system waiting for download to finish,
        // we shall compensate data consumer for that elapsed real time with tantamount simulation time of data:
        auto elapsedSimlTime = std::chrono::milliseconds(TimeSetting::getInstance().pastMilli(true)); // take simulation speed (experimentSpeed) into account
        auto adjustedDPDC = std::chrono::duration_cast<decltype(elapsedSimlTime)>(::DURATION_PER_DATA_CHUNK); // unify the time units
        if (elapsedSimlTime.count() > (adjustedDPDC.count() / 2)) { // it's called "considerably" lagged iff. lag at least half of the duration per chunk
            auto numChunksToCoverElapsedTime = static_cast<long>(elapsedSimlTime.count() / adjustedDPDC.count()) + 1;
            while (numChunksToCoverElapsedTime--)
                requestOnce(&startTime);
        }

        // to guarentee a smooth data streaming: supplier shall always keep some safe amount of data ahead of consumer in buffer
        requestOnce(&startTime);

        // begin periodic streaming:
        do {
            requestOnce(&startTime);
            std::this_thread::sleep_for(::DURATION_PER_DATA_CHUNK / experimentSpeed);
        } while (s_isRequestingData && startTime < endTime);
    }

    FIXInitiator::getInstance()->disconnectDatafeedEngine();
}

int main(int ac, char* av[])
{
    char tz[] = "TZ=America/New_York"; // set time zone to New York
    putenv(tz);

    /**
     * @brief Centralizes and classifies all necessary parameters and
     * hides them behind one variable to ease understanding and debugging.
     */
    struct {
        std::string configDir;
        std::string cryptoKey;
        std::string simulationDate;
        std::string simulationStartTime;
        std::string simulationEndTime;
        struct { // timeout settings
            using min_t = std::chrono::minutes::rep;
            bool isSet;
            min_t minutes;
        } timer;
        bool isVerbose;
        bool isManualInput;
    } params = {
        "/usr/local/share/shift/MatchingEngine/", // default installation folder for configuration
        "SHIFT123", // built-in initial crypto key used for encrypting dbLogin.txt
        "",
        "",
        "",
        {
            false,
            0,
        },
        false,
        false,
    };

    po::options_description desc("\nUSAGE: ./MatchingEngine [options] <args>\n\n\tThis is the MatchingEngine.\n\tThe server connects with DatafeedEngine and BrokerageCenter instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_KEY ",k", po::value<std::string>(), "key of " CSTR_DBLOGIN_TXT " file") //
        (CSTR_DATE ",d", po::value<std::string>(), "simulation date") //
        (CSTR_STARTTIME ",b", po::value<std::string>(), "simulation start time (default: 09:30:00)") //
        (CSTR_ENDTIME ",e", po::value<std::string>(), "simulation end time (default 16:00:00)") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_MANUAL ",m", "set manual input of all parameters") //
        ; // add_options

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);
    } catch (const boost::program_options::error& e) {
        cerr << COLOR_ERROR "ERROR: " << e.what() << NO_COLOR << endl;
        return 1;
    } catch (...) {
        cerr << COLOR_ERROR "ERROR: Exception of unknown type!" NO_COLOR << endl;
        return 2;
    }

    if (vm.count(CSTR_HELP)) {
        cout << '\n'
             << desc << '\n'
             << endl;
        return 0;
    }

    if (vm.count(CSTR_VERBOSE)) {
        params.isVerbose = true;
    }
    voh_t voh(cout, params.isVerbose);

    if (vm.count(CSTR_CONFIG)) {
        params.configDir = vm[CSTR_CONFIG].as<std::string>();
        cout << COLOR "'config' directory was set to "
             << params.configDir << ".\n" NO_COLOR << endl;
    } else {
        cout << COLOR "'config' directory was not set. Using default config dir: " << params.configDir << NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_KEY)) {
        params.cryptoKey = vm[CSTR_KEY].as<std::string>();
    } else {
        cout << COLOR "The built-in initial key 'SHIFT123' is used for reading encrypted login files." NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_DATE)) {
        params.simulationDate = vm[CSTR_DATE].as<std::string>();
    }

    if (vm.count(CSTR_STARTTIME)) {
        params.simulationStartTime = vm[CSTR_STARTTIME].as<std::string>();
    }

    if (vm.count(CSTR_ENDTIME)) {
        params.simulationEndTime = vm[CSTR_ENDTIME].as<std::string>();
    }

    if (vm.count(CSTR_TIMEOUT)) {
        params.timer.minutes = vm[CSTR_TIMEOUT].as<decltype(params.timer)::min_t>();
        if (params.timer.minutes > 0)
            params.timer.isSet = true;
        else {
            cout << COLOR "Note: The timeout option is ignored because of the given value." NO_COLOR << '\n'
                 << endl;
        }
    }

    if (vm.count(CSTR_MANUAL)) {
        params.isManualInput = true;
    }

    std::string configFile = params.configDir + "config.txt";
    std::string dateString = "2018-12-17";
    std::string startTimeString = "09:30:00";
    std::string endTimeString = "16:00:00";
    int experimentSpeed = 1;
    std::vector<std::string> symbols;

    if (!params.isManualInput) {
        if (!fileConfigMode(configFile, dateString, startTimeString, endTimeString, experimentSpeed, symbols)) {
            return 3;
        }
    } else {
        inputConfigMode(dateString, startTimeString, endTimeString, experimentSpeed, symbols);
    }

    if (!params.simulationDate.empty()) {
        dateString = params.simulationDate;
    }

    if (!params.simulationStartTime.empty()) {
        startTimeString = params.simulationStartTime;
    }

    if (!params.simulationEndTime.empty()) {
        endTimeString = params.simulationEndTime;
    }

    std::string requestID = dateString + " :: " + std::to_string(symbols.size());
    boost::posix_time::ptime startTime(boost::posix_time::time_from_string(dateString + " " + startTimeString));
    boost::posix_time::ptime endTime(boost::posix_time::time_from_string(dateString + " " + endTimeString));

    // create Stock Market List and Stock Market objects
    for (auto& symbol : symbols) {
        StockMarketList::getInstance().insert(std::pair<std::string, StockMarket>(symbol, { symbol }));
    }

    if (StockMarketList::getInstance().size() != symbols.size()) {
        cout << "Error during stock list creation!" << endl;
        return 4;
    }

    // begin Stock Market threads
    int numOfStockMarkets = StockMarketList::getInstance().size();
    std::vector<std::thread> stockMarketThreadList(numOfStockMarkets);
    {
        int i = 0;

        for (auto& stockMarketEntry : StockMarketList::getInstance()) {
            stockMarketThreadList[i] = std::thread(std::ref(stockMarketEntry.second));
            ++i;
        }
    }
    cout << endl
         << "A total of " << numOfStockMarkets << " Stock Markets are ready in the Matching Engine." << endl
         << "Waiting for orders..." << endl
         << endl;

    // configure and start global clock
    TimeSetting::getInstance().initiate(startTime, experimentSpeed);
    TimeSetting::getInstance().setStartTime();

    // request data chunks in the background
    std::thread dataRequester(&::s_requestDatafeedEngineData, params.configDir + "initiator.cfg", params.isVerbose, params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT, std::move(requestID), std::move(startTime), std::move(endTime), std::move(symbols), ::DURATION_PER_DATA_CHUNK.count(), experimentSpeed);

    // initiate Brokerage Center connection
    FIXAcceptor::getInstance()->connectBrokerageCenter(params.configDir + "acceptor.cfg", params.isVerbose, params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    // create 'done' file in ~/.shift/MatchingEngine to signalize shell that service is done loading
    // (directory is also created if it does not exist)
    const char* homeDir;
    if ((homeDir = getenv("HOME")) == nullptr) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    std::string servicePath { homeDir };
    servicePath += "/.shift/MatchingEngine";
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
                         << COLOR_PROMPT "The MatchingEngine is running. (Enter 'T' to stop)" NO_COLOR << '\n'
                         << endl;
                    voh_t { cout, params.isVerbose, true };

                    char cmd = cin.get(); // wait
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
                    if ('T' == cmd || 't' == cmd)
                        return;
                }
            })
            .get(); // this_thread will wait for user terminating acceptor.
    }

    // close program
    FIXAcceptor::getInstance()->disconnectBrokerageCenter();

    ::s_isRequestingData = false; // to terminate data requester
    if (dataRequester.joinable())
        dataRequester.join(); // wait for termination

    StockMarketList::s_isTimeout = true;
    for (int i = 0; i < numOfStockMarkets; ++i) {
        stockMarketThreadList[i].join();
    }

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
        cout << endl;
    }

    return 0;
}
