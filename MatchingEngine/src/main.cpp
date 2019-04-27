#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Parameters.h"
#include "Stock.h"
#include "TimeSetting.h"
#include "threadFunction.h"

#include <atomic>
#include <future>

#include <boost/program_options.hpp>

#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Options.h>

using namespace std::chrono_literals;

/* PROGRAM OPTIONS */
#define CSTR_HELP \
    "help"
#define CSTR_CONFIG \
    "config"
#define CSTR_VERBOSE \
    "verbose"
#define CSTR_DATE \
    "date"
#define CSTR_MANUAL \
    "manual"
#define CSTR_TIMEOUT \
    "timeout"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

std::atomic<bool> timeout{ false };

int main(int ac, char* av[])
{
    char tz[] = "TZ=America/New_York"; // Set time zone to New York
    putenv(tz);

    /**
     * @brief Centralizes and classifies all necessary parameters and
     * hides them behind one variable to ease understanding and debugging.
     */
    struct {
        std::string configDir;
        bool isVerbose;
        std::string simulationDate;
        bool isManualInput;
        struct { // timeout settings
            using min_t = std::chrono::minutes::rep;
            bool isSet;
            min_t minutes;
        } timer;
    } params = {
        "/usr/local/share/shift/MatchingEngine/", // default installation folder for configuration
        false,
        "",
        false,
        {
            false,
            400,
        },
    };

    po::options_description desc("\nUSAGE: ./MatchingEngine [options] <args>\n\n\tThis is the MatchingEngine.\n\tThe server connects with DatafeedEngine and BrokerageCenter instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_DATE ",d", po::value<std::string>(), "simulation date") //
        (CSTR_MANUAL ",m", "set manual input of all parameters") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
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

    if (vm.count(CSTR_CONFIG)) {
        params.configDir = vm[CSTR_CONFIG].as<std::string>();
        cout << COLOR "'config' directory was set to "
             << params.configDir << ".\n" NO_COLOR << endl;
    } else {
        cout << COLOR "'config' directory was not set. Using default config dir: " << params.configDir << NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_VERBOSE)) {
        params.isVerbose = true;
    }

    voh_t voh(cout, params.isVerbose);

    if (vm.count(CSTR_DATE)) {
        params.simulationDate = vm[CSTR_DATE].as<std::string>();
    }

    if (vm.count(CSTR_MANUAL)) {
        params.isManualInput = true;
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

    std::string requestID = dateString + " :: " + std::to_string(symbols.size());
    boost::posix_time::ptime startTime(boost::posix_time::time_from_string(dateString + " " + startTimeString));
    boost::posix_time::ptime endTime(boost::posix_time::time_from_string(dateString + " " + endTimeString));

    /*
     * @brief  Create Stock List and Stock Market objects
     */
    for (auto& symbol : symbols) {
        StockList::getInstance().insert(std::pair<std::string, Stock>(symbol, { symbol }));

        // TODO: This should be done in the DE
        // Transform symbol's punctuation (if any) before passing to Datafeed Engine
        for (unsigned int j = 0; j < symbol.size(); ++j) {
            if (symbol[j] == '.')
                symbol[j] = '_';
        }
    }
    if (StockList::getInstance().size() != symbols.size()) {
        cout << "Error during stock list creation!" << endl;
        return 4;
    }

    /*
     * @brief  Begin Stock Market threads
     */
    int numOfStocks = StockList::getInstance().size();
    std::vector<std::thread> stockMarketThreadList(numOfStocks);
    {
        int i = 0;
        auto& stockList = StockList::getInstance();
        for (auto thisStock = stockList.begin(); thisStock != stockList.end(); ++thisStock) {
            stockMarketThreadList[i++] = std::thread(createStockMarket, thisStock->first);
        }
    }
    cout << endl
         << "A total of " << numOfStocks << " stocks are ready in the Matching Engine." << endl
         << "Waiting for quotes..." << endl
         << endl;

    /*
     * @brief  Initiate FIX connections
     */
    FIXAcceptor::getInstance()->connectBrokerageCenter(params.configDir + "acceptor.cfg");
    FIXInitiator::getInstance()->connectDatafeedEngine(params.configDir + "initiator.cfg");

    /*
     * @brief  Send request to Datafeed Engine for TRTH data and wait until data is ready
     */
    // TODO: We should also send DURATION_PER_DATA_CHUNK to DE
    FIXInitiator::getInstance()->sendSecurityListRequestAwait(requestID, startTime, endTime, symbols);

    /*
     * @brief  Configure and start global clock
     */
    TimeSetting::getInstance().initiate(startTime, experimentSpeed);
    TimeSetting::getInstance().setStartTime();

    /*
     * @brief  Request data chunks in the background
     */
    std::thread(
        [startTime, endTime, experimentSpeed]() mutable {
            // Make sure to always keep at least DURATION_PER_DATA_CHUNK of data ahead in buffer
            FIXInitiator::getInstance()->sendNextDataRequest();
            startTime += boost::posix_time::seconds(::DURATION_PER_DATA_CHUNK.count());

            do {
                FIXInitiator::getInstance()->sendNextDataRequest();
                startTime += boost::posix_time::seconds(::DURATION_PER_DATA_CHUNK.count());

                std::this_thread::sleep_for(::DURATION_PER_DATA_CHUNK / experimentSpeed);
            } while (startTime < endTime);

            std::this_thread::sleep_for(::DURATION_PER_DATA_CHUNK / experimentSpeed);
        })
        .detach(); // run in background

    /*
     * @brief   Running in background
     */
    if (params.timer.isSet) {
        cout.clear();
        cout << '\n'
             << COLOR_PROMPT "Timer begins ( " << params.timer.minutes << " mins )..." NO_COLOR << '\n'
             << endl;

        voh_t{ cout, params.isVerbose, true };
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
                    voh_t{ cout, params.isVerbose, true };

                    char cmd = cin.get(); // wait
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
                    if ('T' == cmd || 't' == cmd)
                        return;
                }
            })
            .get(); // this_thread will wait for user terminating acceptor.
    }

    /*
     * @brief   Close program
     */
    FIXInitiator::getInstance()->disconnectDatafeedEngine();
    FIXAcceptor::getInstance()->disconnectBrokerageCenter();
    timeout = true;
    for (int i = 0; i < numOfStocks; i++) {
        stockMarketThreadList[i].join();
    }

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
    }

    return 0;
}
