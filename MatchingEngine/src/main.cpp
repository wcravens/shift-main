#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Stock.h"
#include "globalvariables.h"
#include "threadFunction.h"

#include <atomic>
#include <thread>

#include <boost/program_options.hpp>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

/* PROGRAM OPTIONS */
#define CSTR_HELP \
    "help"
#define CSTR_MANUAL \
    "manual"
#define CSTR_CONFIG \
    "config"
#define CSTR_DATE \
    "date"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;

std::atomic<bool> timeout(false);
std::map<std::string, Stock> stocklist;

int main(int ac, char* av[])
{
    /**
     * @brief Centralizes and classifies all necessary parameters and
     * hides them behind one variable to ease understanding and debugging.
     */
    struct {
        bool isManualInput;
        std::string configDir;
        std::string simulationDate;
    } params = {
        false,
        "/usr/local/share/SHIFT/MatchingEngine/", // default installation folder for configuration
    };

    po::options_description desc("\nUSAGE: ./MatchingEngine [options] <args>\n\n\tThis is the MatchingEngine.\n\tThe server connects with DatafeedEngine and BrokerageCenter instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_MANUAL ",m", "set manual input of all parameters") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_DATE ",d", po::value<std::string>(), "simulation date") //
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
        return 1;
    }

    if (vm.count(CSTR_MANUAL)) {
        params.isManualInput = true;
    }

    if (vm.count(CSTR_CONFIG)) {
        params.configDir = vm[CSTR_CONFIG].as<std::string>();
        cout << COLOR "'config' directory was set to "
             << params.configDir << ".\n" NO_COLOR << endl;
    } else {
        cout << COLOR "'config' directory was not set. Using default config dir: " << params.configDir << NO_COLOR << '\n'
             << endl;
    }

    if (vm.count(CSTR_DATE)) {
        params.simulationDate = vm[CSTR_DATE].as<std::string>();
    }

    std::string configfile = params.configDir + "config.txt";
    std::string date = "2018-03-15";
    std::string stime = "09:30:00";
    std::string etime = "16:00:00";
    int experiment_speed = 1;

    std::vector<std::string> symbols;

    if (!params.isManualInput) {
        if (!fileConfig_mode(configfile, date, stime, etime, experiment_speed, symbols))
            return -1; // system stop
    } else {
        inputConfig_mode(date, stime, etime, experiment_speed, symbols);
    }

    if (!params.simulationDate.empty()) {
        date = params.simulationDate;
    }

    timepara.initiate(date, stime, etime, experiment_speed);
    boost::posix_time::ptime pt_start(boost::posix_time::time_from_string(date + " " + stime));
    boost::posix_time::ptime pt_end(boost::posix_time::time_from_string(date + " " + etime));
    std::string requestID = date + " :: " + std::to_string(symbols.size());
    //initiate connection to clients
    FIXAcceptor fixtoclient;
    //initiate connection to DB
    FIXInitiator fixtoDB;
    fixtoDB.connectDatafeedEngine(params.configDir + "initiator.cfg");
    cout << "Please wait for ready" << endl;
    sleep(3);
    getchar();

    //send request to database, and create stock;
    for (unsigned int i = 0; i < symbols.size(); ++i) {
        std::string symbol;
        symbol = symbols[i];
        fixtoclient.symbollist.push_back(symbol);
        Stock newstock;
        newstock.setstockname(symbol);
        //store the new stock into stocklist
        stocklist.insert(std::pair<std::string, Stock>(newstock.getstockname(), newstock));

        for (unsigned int j = 0; j < symbols[i].size(); ++j) {
            if (symbols[i][j] == '.')
                symbols[i][j] = '_';
        }
    }
    if (symbols.size() != stocklist.size()) {
        cout << "Error in creating Stock to stocklist" << endl;
        return 1;
    }
    fixtoclient.symbollist.sort();
    //send request to DatafeedEngine for TRTH data
    fixtoDB.sendSecurityList(requestID, pt_start, pt_end, symbols);
    cout << "Please wait for the database signal until TRTH data ready..." << endl;
    getchar();
    getchar();
    fixtoDB.sendMarketDataRequest();
    //option to start the exchange
    cout << endl
         << endl
         << endl
         << "Waiting for database downloading and saving data of all the stocks..." << endl
         << endl
         << endl;

    while (1) {
        //int startExchange = 0;
        cout << "To start exchange, please press '1'" << endl
             << "Please wait for the history data from database server, then type '1', to start this exchange..." << endl;

        //if(cin>>startExchange){if(startExchange==1) break;}
        //else{cin.clear();cin.ignore(255,'\n');}
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
    timepara.set_start_time();
    //begin the matching thread
    int stocknumber = stocklist.size();
    cout << "Total " << stocknumber << " stocks are ready in the Matching Engine"
         << endl
         << "Waiting for quotes..." << endl;
    //std::thread  stockthreadlist[stocknumber];
    std::vector<std::thread> stockthreadlist(stocknumber);
    {
        int i = 0;
        for (std::map<std::string, Stock>::iterator thisStock = stocklist.begin(); thisStock != stocklist.end(); thisStock++) {
            stockthreadlist[i] = std::thread(createStockMarket, thisStock->first);
            ++i;
        }
    }

    fixtoclient.connectBrokerageCenter(params.configDir + "acceptor.cfg");

    //sleep the main thread for set of time until exchange close time
    pt_start += boost::posix_time::minutes(15);
    while (pt_start < pt_end) {
        fixtoDB.sendMarketDataRequest();
        pt_start += boost::posix_time::minutes(15);
        std::this_thread::sleep_for(15min);
    }
    std::this_thread::sleep_for(15min);

    std::this_thread::sleep_for(1min);
    timeout = true;
    //{
    //    boost::posix_time::time_duration timelaps=boost::posix_time::duration_from_string(etime)-boost::posix_time::duration_from_string(stime)
    //                                                +boost::posix_time::minutes(5);
    //    std::this_thread::sleep_for(timelaps.total_seconds() * 1s);
    //    timeout=true;
    //}

    fixtoDB.disconnectDatafeedEngine();
    fixtoclient.disconnectBrokerageCenter();
    for (int i = 0; i < stocknumber; i++) {
        stockthreadlist[i].join();
    }

    cout << "Success!" << endl;

    return 0;
}
