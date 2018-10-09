/* 
** This file contains the main function of BrokerageCenter
**
** Function List:
** 1. parse the program options
** 2. connect to database
** 3. start FIXInitiator and FIXAcceptor
** 4. run this program in the background
*/

#include "BCDocuments.h"
#include "DBConnector.h"
#include "FIXAcceptor.h"
#include "FIXInitiator.h"

#include <atomic>

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
#define CSTR_TIMEOUT \
    "timeout"
#define CSTR_VERBOSE \
    "verbose"
#define CSTR_RESET \
    "reset"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

static std::atomic<bool> s_isBroadcasting{ true };

/* 
 * @brief   Function to broadcast order books, for broadcast order book thread
 */
static void s_broadcastOrderBook() // broadcasting the whole orderbook
{
    while (::s_isBroadcasting) {
        // broadcast the full order book of every stock every 1s (forcing a refresh in the client side)
        std::this_thread::sleep_for(1s);
        BCDocuments::instance()->broadcastStocks();
    }
}

int main(int ac, char* av[])
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
        "/usr/local/share/SHIFT/BrokerageCenter/", // default installation folder for configuration
        "SHIFT123", // built-in initial crypto key used for encrypting dbLogin.txt
        {
            false,
            0,
        },
        false,
    };

    po::options_description desc("\nUSAGE: ./BrokerageCenter [options] <args>\n\n\tThis is the brokerage application.\n\tThe server connects with MatchingEngine and the WebClient instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_KEY ",k", po::value<std::string>(), "key of " CSTR_DBLOGIN_TXT " file") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_RESET ",r", "reset clients' portfolio databases") //
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

    if (vm.count(CSTR_TIMEOUT)) {
        params.timer.minutes = vm[CSTR_TIMEOUT].as<decltype(params.timer)::min_t>();
        if (params.timer.minutes > 0)
            params.timer.isSet = true;
        else {
            cout << COLOR "Note: The timeout option is ignored because of the given value." NO_COLOR << '\n'
                 << endl;
        }
    }

    if (vm.count(CSTR_VERBOSE)) {
        params.isVerbose = true;
    }

    voh_t voh(cout, params.isVerbose);

    /*
     * @brief   Try to connect database
     */
    DBConnector::instance()->init(params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    if (!DBConnector::instance()->connectDB()) {
        cout << "DB Error: Fail to connection database." << endl;
    } else {
        cout << "Success to connect database." << endl;

        if (vm.count(CSTR_RESET)) {
            DBConnector::instance()->doQuery("DROP TABLE portfolio_summary CASCADE", COLOR_ERROR "ERROR: Failed to drop portfolio_summary" NO_COLOR);
            DBConnector::instance()->doQuery("DROP TABLE portfolio_items CASCADE", COLOR_ERROR "ERROR: Failed to drop portfolio_items" NO_COLOR);
            DBConnector::instance()->connectDB();
        }
    }

    DBConnector::instance()->createClients("XYZ");

    /*
     * @brief   Try to connect Matching Engine and waiting for client
     */
    FIXInitiator::instance()->connectMatchingEngine(params.configDir + "initiator.cfg", params.isVerbose);
    FIXAcceptor::instance()->connectClients(params.configDir + "acceptor.cfg", params.isVerbose);

    /*
     * @brief   Create a broadcaster to broadcast all orderbook
     */
    std::thread broadcaster(&::s_broadcastOrderBook);

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
                         << COLOR_PROMPT "The BrokerageCenter is running. (Enter 'T' to stop)" NO_COLOR << '\n'
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
    ::s_isBroadcasting = false; // to terminate broadcaster
    if (broadcaster.joinable())
        broadcaster.join(); // wait for termination

    FIXAcceptor::instance()->disconnectClients();
    FIXInitiator::instance()->disconnectMatchingEngine();

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
    }
}
