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

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;

std::atomic<bool> timeout(false);
std::map<std::string, Stock> stocklist;
std::string CONFIG_FOLDER = "/usr/local/share/SHIFT/MatchingEngine/";

int main(int ac, char* av[])
{
    try {

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("config", po::value<std::string>(), "set config directory");

        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 1;
        }

        if (vm.count("config")) {
            CONFIG_FOLDER = vm["config"].as<std::string>();
            cout << "config directory was set to "
                 << vm["config"].as<std::string>() << ".\n";
        } else {
            cout << "config directory was not set. Using default config dir: " << CONFIG_FOLDER << endl;
        }
    } catch (const boost::program_options::error& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        cerr << "Exception of unknown type!\n";
    }

    std::string configfile = CONFIG_FOLDER + "config.txt";
    std::string date = "2014-10-08"; //2014-03-11
    std::string stime = "09:30:00";
    std::string etime = "16:00:00";
    double experiment_speed = 1;
    cout << "FE800 Project: Test Bed For High Frequency Trading" << endl;
    cout << "The Machine Engine" << endl;
    cout << "If want a debug mode, please type 0: " << endl;
    cout << "Or complete the 'config.txt' and type '1' to start the program..." << endl;
    int config_mode = 1;
    //cin>>config_mode;
    //if(!cin){
    //    cin.clear();
    //    cin.ignore(255);
    //    return -1;
    //}
    std::vector<std::string> symbols;
    switch (config_mode) {
    case 0:
        debug_mode(symbols, date, stime, etime, experiment_speed);
        break;
    case 1:
        if (!fileConfig_mode(configfile, symbols, date, stime, etime, experiment_speed))
            return -1; //system stop
        break;
    default:
        inputConfig_mode(symbols, date, stime, etime, experiment_speed);
    }

    timepara.initiate(date, stime, etime, experiment_speed);
    boost::posix_time::ptime pt_start(boost::posix_time::time_from_string(date + " " + stime));
    boost::posix_time::ptime pt_end(boost::posix_time::time_from_string(date + " " + etime));
    std::string requestID = date + " :: " + std::to_string(symbols.size());
    //initiate connection to clients
    FIXAcceptor fixtoclient;
    //initiate connection to DB
    FIXInitiator fixtoDB;
    fixtoDB.connectDatafeedEngine(CONFIG_FOLDER + "initiator.cfg");
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

    fixtoclient.connectBrokerageCenter(CONFIG_FOLDER + "acceptor.cfg");

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

    cout << timepara.timestamp_inner() << endl;
    cout << "Success!" << endl;

    return 0;
}
