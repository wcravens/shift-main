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

#include <algorithm>
#include <atomic>

#include <boost/program_options.hpp>

#include <shift/miscutils/crypto/Encryptor.h>
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
#define CSTR_USERNAME \
    "username"
#define CSTR_PASSWORD \
    "password"
#define CSTR_INFO \
    "info"
#define CSTR_SUPER \
    "super"

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
        // broadcast the full order book of every stock every 30s (forcing a refresh in the client side)
        std::this_thread::sleep_for(30s);
        BCDocuments::getInstance()->broadcastStocks();
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
        struct { // user creation settings
            std::string userName; // needs also password & info
            std::string password;
            std::vector<std::string> info;
        } user;
    } params = {
        "/usr/local/share/shift/BrokerageCenter/", // default installation folder for configuration
        "SHIFT123", // built-in initial crypto key used for encrypting dbLogin.txt
        {
            false,
            0,
        },
        false,
    };

    po::options_description desc("\nUSAGE: ./BrokerageCenter [options] <args>\n\n\tThis is the BrokerageCenter.\n\tThe server connects with MatchingEngine and client instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_KEY ",k", po::value<std::string>(), "key of " CSTR_DBLOGIN_TXT " file") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_RESET ",r", "reset clients' portfolio databases") //
        (CSTR_USERNAME ",u", po::value<std::string>(), "name of the new user") //
        (CSTR_PASSWORD ",p", po::value<std::string>(), "password of the new user") //
        (CSTR_INFO ",i", po::value<std::vector<std::string>>()->multitoken(), "<first name>  <last name>  <email>") //
        (CSTR_SUPER ",s", "is super user, requires -u present") //
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

    if (vm.count(CSTR_TIMEOUT)) {
        params.timer.minutes = vm[CSTR_TIMEOUT].as<decltype(params.timer)::min_t>();
        if (params.timer.minutes > 0)
            params.timer.isSet = true;
        else {
            cout << COLOR "Note: The timeout option is ignored because of the given value." NO_COLOR << '\n'
                 << endl;
        }
    }

    const auto optsUPI = { CSTR_USERNAME, CSTR_PASSWORD, CSTR_INFO };
    auto isIncluded = [&vm](auto* opt) { return vm.count(opt); };

    if (std::any_of(optsUPI.begin(), optsUPI.end(), isIncluded) || vm.count(CSTR_SUPER)) {
        if (!std::all_of(optsUPI.begin(), optsUPI.end(), isIncluded)) {
            cout << COLOR_ERROR "ERROR: The new user options are not sufficient."
                                " Please provide -u, -p, and -i at the same time." NO_COLOR
                 << '\n'
                 << endl;
            return 3;
        }

        params.user.userName = vm[CSTR_USERNAME].as<std::string>();

        std::istringstream iss(vm[CSTR_PASSWORD].as<std::string>());
        shift::crypto::Encryptor enc;
        iss >> enc >> params.user.password;

        params.user.info = vm[CSTR_INFO].as<std::vector<std::string>>();
        if (params.user.info.size() != 3) {
            cout << COLOR_ERROR "ERROR: The new user information is not complete (" << params.user.info.size() << " info are provided) !\n" NO_COLOR
                 << endl;
            return 4;
        }
    }

    DBConnector::getInstance()->init(params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    while (true) {
        if (!DBConnector::getInstance()->connectDB()) {
            cout.clear();
            cout << COLOR_ERROR "DB ERROR: Failed to connect database." NO_COLOR << endl;
            cout << "\tRetry ('Y') connection to database ? : ";
            voh_t{ cout, params.isVerbose, true };

            char cmd = cin.get();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
            if ('Y' != cmd && 'y' != cmd)
                return 1;
        } else {
            cout << "DB connection OK.\n"
                 << endl;

            if (vm.count(CSTR_RESET)) {
                vm.erase(CSTR_RESET);

                cout << COLOR_WARNING "Resetting the databases..." NO_COLOR << endl;
                DBConnector::getInstance()->doQuery("DROP TABLE portfolio_summary CASCADE", COLOR_ERROR "ERROR: Failed to drop [ portfolio_summary ]." NO_COLOR);
                DBConnector::getInstance()->doQuery("DROP TABLE portfolio_items CASCADE", COLOR_ERROR "ERROR: Failed to drop [ portfolio_items ]." NO_COLOR);
                continue;
            }

            if (vm.count(CSTR_USERNAME)) { // add user ?
                const auto& fname = params.user.info[0];
                const auto& lname = params.user.info[1];
                const auto& email = params.user.info[2];

                const auto res = DBConnector::s_readRowsOfField("SELECT id FROM traders WHERE username = '" + params.user.userName + "';");
                if (res.size()) {
                    cout << COLOR_WARNING "The user " << params.user.userName << " already exists!" NO_COLOR << endl;
                    return 1;
                }

                const auto insert = "INSERT INTO traders (username, password, firstname, lastname, email, super) VALUES ('"
                    + params.user.userName + "','"
                    + params.user.password + "','"
                    + fname + "','" + lname + "','" + email // info
                    + (vm.count(CSTR_SUPER) > 0 ? "',TRUE);" : "',FALSE);");
                if (DBConnector::getInstance()->doQuery(insert, COLOR_ERROR "ERROR: Failed to insert user into DB!\n" NO_COLOR)) {
                    cout << COLOR "User " << params.user.userName << " was successfully inserted." NO_COLOR << endl;
                    return 0;
                }

                return 5;
            }

            break; // finished connection and continues
        }
    }

    DBConnector::getInstance()->createUsers("XYZ");

    /*
     * @brief   Try to connect Matching Engine and waiting for client
     */
    FIXInitiator::getInstance()->connectMatchingEngine(params.configDir + "initiator.cfg", params.isVerbose);
    FIXAcceptor::getInstance()->connectClientComputers(params.configDir + "acceptor.cfg", params.isVerbose);

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

    FIXAcceptor::getInstance()->disconnectClientComputers();
    FIXInitiator::getInstance()->disconnectMatchingEngine();

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
    }
}
