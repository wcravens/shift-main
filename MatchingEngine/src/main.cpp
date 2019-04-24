#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Stock.h"
#include "TimeSetting.h"
#include "threadFunction.h"

#include <atomic>
#include <thread>

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

std::atomic<bool> timeout(false);

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
        using min_t = std::chrono::minutes::rep;
        min_t minutes;
    } params = {
        "/usr/local/share/shift/MatchingEngine/", // default installation folder for configuration
        false,
        "",
        false,
        400, // mins
    };

    po::options_description desc("\nUSAGE: ./MatchingEngine [options] <args>\n\n\tThis is the MatchingEngine.\n\tThe server connects with DatafeedEngine and BrokerageCenter instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_DATE ",d", po::value<std::string>(), "simulation date") //
        (CSTR_MANUAL ",m", "set manual input of all parameters") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
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

    if (vm.count(CSTR_TIMEOUT))
        params.minutes = vm[CSTR_TIMEOUT].as<decltype(params)::min_t>();

    std::string configFile = params.configDir + "config.txt";
    std::string date = "2018-12-17";
    std::string stime = "09:30:00";
    std::string etime = "16:00:00";
    int experimentSpeed = 1;

    std::vector<std::string> symbols;

    if (!params.isManualInput) {
        if (!fileConfigMode(configFile, date, stime, etime, experimentSpeed, symbols))
            return 3;

    } else {
        inputConfigMode(date, stime, etime, experimentSpeed, symbols);
    }

    if (!params.simulationDate.empty()) {
        date = params.simulationDate;
    }

    TimeSetting::getInstance().initiate(date, stime, experimentSpeed);
    boost::posix_time::ptime ptimeStart(boost::posix_time::time_from_string(date + " " + stime));
    boost::posix_time::ptime ptimeEnd(boost::posix_time::time_from_string(date + " " + etime));
    std::string requestID = date + " :: " + std::to_string(symbols.size());

    // Initiate connection with DE
    FIXInitiator::getInstance()->connectDatafeedEngine(params.configDir + "initiator.cfg");
    sleep(3);

    // Create stock list and Stock objects
    for (unsigned int i = 0; i < symbols.size(); ++i) {
        auto& symbol = symbols[i];

        FIXAcceptor::getInstance()->addSymbol(symbol);

        Stock stock(symbol);
        StockList::getInstance().insert(std::pair<std::string, Stock>(symbol, stock));

        // transform symbol's punctuation(if any) before passing to DatafeedEngine
        for (unsigned int j = 0; j < symbol.size(); ++j) {
            if (symbol[j] == '.')
                symbol[j] = '_';
        }
    }
    if (symbols.size() != StockList::getInstance().size()) {
        cout << "Error in creating Stock to StockList" << endl;
        return 4;
    }

    // Send request to DatafeedEngine for TRTH data and wait until data is ready
    FIXInitiator::getInstance()->sendSecurityListRequestAwait(requestID, ptimeStart, ptimeEnd, symbols);

    // Get the time offset in current day
    TimeSetting::getInstance().setStartTime();

    // Begin Matching Engine threads
    int numOfStock = StockList::getInstance().size();
    cout << "Total " << numOfStock << " stocks are ready in the Matching Engine"
         << endl
         << "Waiting for quotes..." << endl;
    std::vector<std::thread> stockThreadList(numOfStock);
    {
        int i = 0;
        auto& stockList = StockList::getInstance();
        for (auto thisStock = stockList.begin(); thisStock != stockList.end(); thisStock++) {
            stockThreadList[i] = std::thread(createStockMarket, thisStock->first);
            ++i;
        }
    }

    // Initiate connection with BC
    FIXAcceptor::getInstance()->connectBrokerageCenter(params.configDir + "acceptor.cfg");

    cout.clear();
    cout << '\n'
         << COLOR_PROMPT "Timer begins ( " << params.minutes << " minutes )..." NO_COLOR << '\n'
         << endl;

    voh_t{ cout, params.isVerbose, true };
    std::this_thread::sleep_for(params.minutes * 1min);

    FIXInitiator::getInstance()->disconnectDatafeedEngine();
    FIXAcceptor::getInstance()->disconnectBrokerageCenter();
    for (int i = 0; i < numOfStock; i++) {
        stockThreadList[i].join();
    }

    cout << "Success!" << endl;

    return 0;
}
