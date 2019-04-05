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

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

std::atomic<bool> timeout(false);
std::map<std::string, Stock> stockList;

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
    } params = {
        "/usr/local/share/shift/MatchingEngine/", // default installation folder for configuration
        true,
        "2018-12-17",
        false,
    };

    po::options_description desc("\nUSAGE: ./MatchingEngine [options] <args>\n\n\tThis is the MatchingEngine.\n\tThe server connects with DatafeedEngine and BrokerageCenter instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_DATE ",d", po::value<std::string>(), "simulation date") //
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

    std::string configFile = params.configDir + "config.txt";
    std::string date = params.simulationDate;
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

    globalTimeSetting.initiate(date, stime, experimentSpeed);
    boost::posix_time::ptime ptimeStart(boost::posix_time::time_from_string(date + " " + stime));
    boost::posix_time::ptime ptimeEnd(boost::posix_time::time_from_string(date + " " + etime));
    std::string requestID = date + " :: " + std::to_string(symbols.size());

    // Initiate connection with DE
    FIXInitiator::getInstance()->connectDatafeedEngine(params.configDir + "initiator.cfg");
    cout << "Please wait for ready" << endl;
    sleep(3);
    getchar();

    // Create stock list and Stock objects
    for (unsigned int i = 0; i < symbols.size(); ++i) {
        std::string symbol;
        symbol = symbols[i];
        FIXAcceptor::getInstance()->addSymbol(symbol);
        Stock newStock;
        newStock.setName(symbol);
        stockList.insert(std::pair<std::string, Stock>(newStock.getName(), newStock));

        for (unsigned int j = 0; j < symbols[i].size(); ++j) {
            if (symbols[i][j] == '.')
                symbols[i][j] = '_';
        }
    }
    if (symbols.size() != stockList.size()) {
        cout << "Error in creating Stock to stockList" << endl;
        return 4;
    }

    // Send request to DatafeedEngine for TRTH data
    FIXInitiator::getInstance()->sendSecurityList(requestID, ptimeStart, ptimeEnd, symbols);
    cout << "Please wait for the database signal until TRTH data ready..." << endl;
    getchar();
    getchar();
    FIXInitiator::getInstance()->sendMarketDataRequest();

    // Option to start the Exchange
    cout << endl
         << endl
         << endl
         << "Waiting for database downloading and saving data of all the stocks..." << endl
         << endl
         << endl;

    while (1) {
        // int startExchange = 0;
        cout << "To start exchange, please press '1'" << endl
             << "Please wait for the history data from database server, then type '1', to start this exchange..." << endl;

        // if (cin >> startExchange) { if (startExchange == 1) break; }
        // else { cin.clear(); cin.ignore(255,'\n'); }
        break;
    }
    cout << endl
         << endl
         << endl
         << "Waiting for database sending first chunk of all the stocks..."
         << endl
         << endl
         << endl;
    // this_thread::sleep_for(120s);

    // Get the time offset in current day
    globalTimeSetting.setStartTime();

    // Begin Matching Engine threads
    int numOfStock = stockList.size();
    cout << "Total " << numOfStock << " stocks are ready in the Matching Engine"
         << endl
         << "Waiting for quotes..." << endl;
    std::vector<std::thread> stockThreadList(numOfStock);
    {
        int i = 0;
        for (std::map<std::string, Stock>::iterator thisStock = stockList.begin(); thisStock != stockList.end(); thisStock++) {
            stockThreadList[i] = std::thread(createStockMarket, thisStock->first);
            ++i;
        }
    }

    // Initiate connection with BC
    FIXAcceptor::getInstance()->connectBrokerageCenter(params.configDir + "acceptor.cfg");

    // Request new market data every 15 minutes
    ptimeStart += boost::posix_time::minutes(15);
    while (ptimeStart < ptimeEnd) {
        FIXInitiator::getInstance()->sendMarketDataRequest();
        ptimeStart += boost::posix_time::minutes(15);
        std::this_thread::sleep_for(15min / experimentSpeed);
    }
    std::this_thread::sleep_for(15min / experimentSpeed);

    std::this_thread::sleep_for(1min);
    timeout = true;

    FIXInitiator::getInstance()->disconnectDatafeedEngine();
    FIXAcceptor::getInstance()->disconnectBrokerageCenter();
    for (int i = 0; i < numOfStock; i++) {
        stockThreadList[i].join();
    }

    cout << "Success!" << endl;

    return 0;
}
