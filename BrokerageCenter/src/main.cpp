#include "BCDocuments.h"
#include "DBConnector.h"
#include "FIXAcceptor.h"
#include "FIXInitiator.h"

#include <algorithm>
#include <atomic>
#if GCC_VERSION < 8
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

#include <pwd.h>

#include <boost/program_options.hpp>

#include <shift/miscutils/crypto/Encryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Options.h>

#include <sys/resource.h>

/* PROGRAM OPTIONS */
#define CSTR_HELP \
    "help"
#define CSTR_CONFIG \
    "config"
#define CSTR_KEY \
    "key"
#define CSTR_DBLOGIN_TXT \
    "dbLogin.txt"
#define CSTR_RESET \
    "reset"
#define CSTR_PFDBREADONLY \
    "readonlyportfolio"
#define CSTR_TIMEOUT \
    "timeout"
#define CSTR_VERBOSE \
    "verbose"
#define CSTR_USERNAME \
    "username"
#define CSTR_PASSWORD \
    "password"
#define CSTR_INFO \
    "info"
#define CSTR_SUPER \
    "super"
#define CSTR_CHANGE_PSW \
    "changepassword"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

static std::atomic<bool> s_isBroadcasting { true };

/*
 * @brief Function to broadcast order books, for broadcast order book thread.
 */
static void s_broadcastOrderBooks()
{
    while (::s_isBroadcasting) {
        // broadcast the full order book of every stock every BROADCAST_ORDERBOOK_PERIOD (forcing a refresh in the client side)
        std::this_thread::sleep_for(::BROADCAST_ORDERBOOK_PERIOD);
        BCDocuments::getInstance()->broadcastOrderBooks();
    }
}

int main(int ac, char* av[])
{
    char tz[] = "TZ=America/New_York"; // set time zone to New York
    putenv(tz);

    /**
     * @brief In UNIX, open sockets are handled with file descriptors.
     * When storing FIX messages AND handling too many client connections,
     * we need to request an increase in the open files allowance for this process:
     * - soft limit = current limit of open files
     *   (1024 in Ubuntu 18.04; 256 in macOS 10.15)
     * - hard limit = maximum number of open files a process may request
     *   (4096 in Ubuntu 18.04; unlimited in macOS 10.15)
     */
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        rlim.rlim_cur = 4096; // soft limit = 4096
        if (rlim.rlim_cur > rlim.rlim_max) {
            rlim.rlim_cur = rlim.rlim_max; // soft limit = hard limit
        }
        setrlimit(RLIMIT_NOFILE, &rlim);
    }

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
            std::string username; // needs also password & info
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
        (CSTR_RESET ",r", "reset client portfolios and trading records") //
        (CSTR_PFDBREADONLY ",o", "is portfolio data in DB read-only") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        (CSTR_USERNAME ",u", po::value<std::string>(), "name of the new user") //
        (CSTR_PASSWORD ",p", po::value<std::string>(), "password of the new user") //
        (CSTR_INFO ",i", po::value<std::vector<std::string>>()->multitoken(), "<first name>  <last name>  <email>") //
        (CSTR_SUPER ",s", "is super user, requires -u present") //
        (CSTR_CHANGE_PSW, "flag for changing password, also requires -u and -p provided") //
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
    auto isProvided = [&vm](auto* opt) { return vm.count(opt); };

    if (!vm.count(CSTR_CHANGE_PSW)
        && (std::any_of(optsUPI.begin(), optsUPI.end(), isProvided) || vm.count(CSTR_SUPER))) {
        if (!std::all_of(optsUPI.begin(), optsUPI.end(), isProvided)) {
            cout << COLOR_ERROR "ERROR: The new "
                 << (vm.count(CSTR_SUPER) ? "super " : "")
                 << "user's options are not sufficient. Please provide -u, -p, and -i at the same time." NO_COLOR
                 << '\n'
                 << endl;
            return 3;
        }

        params.user.username = vm[CSTR_USERNAME].as<std::string>();

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

    DBConnector::s_isPortfolioDBReadOnly = vm.count(CSTR_PFDBREADONLY) > 0;

    DBConnector::getInstance()->init(params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    while (true) {
        if (!DBConnector::getInstance()->connectDB()) {
            cout.clear();
            cout << COLOR_ERROR "DB ERROR: Failed to connect database." NO_COLOR << endl;
            cout << "\tRetry ('Y') connection to database ? : ";
            voh_t { cout, params.isVerbose, true };

            char cmd = cin.get();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip remaining inputs
            if ('Y' != cmd && 'y' != cmd)
                return 5;
        } else {
            cout << "DB connection OK.\n"
                 << endl;

            if (vm.count(CSTR_CHANGE_PSW)) {
                if (!vm.count(CSTR_USERNAME) || !vm.count(CSTR_PASSWORD)) {
                    cout << COLOR_ERROR "ERROR: The password changing options are not sufficient."
                                        " Please provide -u, -p at the same time." NO_COLOR
                         << '\n'
                         << endl;
                    return 6;
                }

                params.user.username = vm[CSTR_USERNAME].as<std::string>();

                std::istringstream iss(vm[CSTR_PASSWORD].as<std::string>());
                shift::crypto::Encryptor enc;
                iss >> enc >> params.user.password;

                if (!DBConnector::getInstance()->doQuery("UPDATE traders SET password = '" + params.user.password + "' WHERE username = '" + params.user.username + "';", COLOR_ERROR "ERROR: Failed to change the user [" + params.user.username + "]'s password!\n" NO_COLOR))
                    return 7;

                cout << COLOR "Password changed." NO_COLOR << endl;
                return 0;
            }

            if (vm.count(CSTR_RESET)) {
                vm.erase(CSTR_RESET);

                cout << COLOR_WARNING "Resetting the databases..." NO_COLOR << endl;
                DBConnector::getInstance()->doQuery("DROP TABLE trading_records CASCADE", COLOR_ERROR "ERROR: Failed to drop [ trading_records ]." NO_COLOR);
                DBConnector::getInstance()->doQuery("DROP TABLE portfolio_summary CASCADE", COLOR_ERROR "ERROR: Failed to drop [ portfolio_summary ]." NO_COLOR);
                DBConnector::getInstance()->doQuery("DROP TABLE portfolio_items CASCADE", COLOR_ERROR "ERROR: Failed to drop [ portfolio_items ]." NO_COLOR);
                continue;
            }

            if (vm.count(CSTR_USERNAME)) { // add user ?
                const auto& fname = params.user.info[0];
                const auto& lname = params.user.info[1];
                const auto& email = params.user.info[2];

                const auto res = shift::database::readRowsOfField(DBConnector::getInstance()->getConn(), "SELECT id FROM traders WHERE username = '" + params.user.username + "';");
                if (res.size()) {
                    cout << COLOR_WARNING "The user " << params.user.username << " already exists!" NO_COLOR << endl;
                    return 0;
                }

                const auto insert = "INSERT INTO traders (username, password, firstname, lastname, email, super) VALUES ('"
                    + params.user.username + "','"
                    + params.user.password + "','"
                    + fname + "','" + lname + "','" + email // info
                    + (vm.count(CSTR_SUPER) > 0 ? "',TRUE);" : "',FALSE);");
                if (DBConnector::getInstance()->doQuery(insert, COLOR_ERROR "ERROR: Failed to insert user into DB!\n" NO_COLOR)) {
                    cout << COLOR "User " << params.user.username << " was successfully inserted." NO_COLOR << endl;
                    return 0;
                }

                return 8;
            }

            break; // finished connection and continues
        }
    }

    // try to connect to Matching Engine
    FIXInitiator::getInstance()->connectMatchingEngine(params.configDir + "initiator.cfg", params.isVerbose, params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    // wait for complete security list
    while (!BCDocuments::s_isSecurityListReady)
        std::this_thread::sleep_for(500ms);

    // try to connect to clients
    FIXAcceptor::getInstance()->connectClients(params.configDir + "acceptor.cfg", params.isVerbose, params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    // create a broadcaster to broadcast all order books
    std::thread broadcaster(&::s_broadcastOrderBooks);

    // create 'done' file in ~/.shift/BrokerageCenter to signalize shell that service is done loading
    // (directory is also created if it does not exist)
    const char* homeDir;
    if ((homeDir = getenv("HOME")) == nullptr) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    std::string servicePath { homeDir };
    servicePath += "/.shift/BrokerageCenter";
#if GCC_VERSION < 8
    std::experimental::filesystem::create_directories(servicePath);
#else
    std::filesystem::create_directories(servicePath);
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
                         << COLOR_PROMPT "The BrokerageCenter is running. (Enter 'T' to stop)" NO_COLOR << '\n'
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
    ::s_isBroadcasting = false; // to terminate broadcaster
    if (broadcaster.joinable())
        broadcaster.join(); // wait for termination

    FIXAcceptor::getInstance()->disconnectClients();
    FIXInitiator::getInstance()->disconnectMatchingEngine();

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
        cout << endl;
    }

    return 0;
}
