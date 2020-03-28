#include "FIXAcceptor.h"
#include "PSQL.h"
#include "TRTHAPI.h"

#if GCC_VERSION < 8
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

#include <pwd.h>

#include <boost/program_options.hpp>

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>
#include <shift/miscutils/terminal/Options.h>

using namespace std::chrono_literals;

#define CSTR_HELP \
    "help"
#define CSTR_CONFIG \
    "config"
#define CSTR_KEY \
    "key"
#define CSTR_DBLOGIN_TXT \
    "dbLogin.txt"
#define CSTR_TRTHLOGIN_JSN \
    "trthLogin.json"
#define CSTR_TIMEOUT \
    "timeout"
#define CSTR_VERBOSE \
    "verbose"

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

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
        struct { // timeout settings
            using min_t = std::chrono::minutes::rep;
            bool isSet;
            min_t minutes;
        } timer;
        bool isVerbose;
    } params = {
        "/usr/local/share/shift/DatafeedEngine/", // default installation folder for configuration
        "SHIFT123", // built-in initial crypto key used for encrypting dbLogin.txt
        {
            false,
            0,
        },
        false,
    };

    po::options_description desc("\nUSAGE: ./DatafeedEngine [options] <args>\n\n\tThis is the DatafeedEngine.\n\tThe server connects with TRTH and MatchingEngine instances and runs in background.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_KEY ",k", po::value<std::string>(), "shared key of " CSTR_DBLOGIN_TXT " and " CSTR_TRTHLOGIN_JSN " files") //
        (CSTR_TIMEOUT ",t", po::value<decltype(params.timer)::min_t>(), "timeout duration counted in minutes. If not provided, user should terminate server with the terminal.") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
        ; // add_options

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);
    } catch (const boost::program_options::error& e) {
        cerr << COLOR_ERROR "error: " << e.what() << NO_COLOR << endl;
        return 1;
    } catch (...) {
        cerr << COLOR_ERROR "error: Exception of unknown type!" NO_COLOR << endl;
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
             << vm[CSTR_CONFIG].as<std::string>() << ".\n" NO_COLOR << endl;
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

    TRTHAPI::s_bTRTHLoginJsonExists = std::ifstream { params.configDir + CSTR_TRTHLOGIN_JSN }.good(); // file exists ?

    // database init
    auto loginPSQL = shift::crypto::readEncryptedConfigFile(params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);
    PSQLManager::createInstance(std::move(loginPSQL)); // already moved-in, don't use loginPSQL thereafter!
    PSQLManager::getInstance().init();

    cout << '\n'
         << COLOR "TRTH is starting..." NO_COLOR << '\n'
         << endl;
    TRTHAPI::createInstance(params.cryptoKey, params.configDir)->start();

    FIXAcceptor::getInstance()->connectMatchingEngine(params.configDir + "acceptor.cfg", params.isVerbose, params.cryptoKey, params.configDir + CSTR_DBLOGIN_TXT);

    // create 'done' file in ~/.shift/DatafeedEngine to signalize shell that service is done loading
    // (directory is also created if it does not exist)
    const char* homeDir;
    if ((homeDir = getenv("HOME")) == nullptr) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    std::string servicePath { homeDir };
    servicePath += "/.shift/DatafeedEngine";
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
                         << COLOR_PROMPT "The DatafeedEngine is running. (Enter 'T' to stop)" NO_COLOR << '\n'
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
    FIXAcceptor::getInstance()->disconnectMatchingEngine();
    TRTHAPI::getInstance()->stop();

    if (params.isVerbose) {
        cout.clear();
        cout << "\nExecution finished. \nPlease press enter to close window: " << flush;
        std::getchar();
        cout << endl;
    }

    return 0;
}
