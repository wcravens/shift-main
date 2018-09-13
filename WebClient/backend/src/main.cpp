#include "MainClient.h"
#include "SHIFTServiceHandler.h"

#include <thread>

#include <boost/program_options.hpp>

#include <shift/coreclient/FIXInitiator.h>

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

/* Abbreviation of NAMESPACE */
namespace po = boost::program_options;
/* 'using' is the same as 'typedef' */
using voh_t = shift::terminal::VerboseOptHelper;

std::string CONFIG_DIR = "/usr/local/SHIFT/WebClient/config/";

int main(int ac, char* av[])
{
    /**
     * @brief Centralizes and classifies all necessary parameters and 
     * hides them behind one variable to ease understanding and debugging. 
     */
    struct {
        std::string configDir;
        bool isVerbose;
    } params = {
        "/usr/local/share/SHIFT/WebClient/", // default installation folder for configuration
        false,
    };

    po::options_description desc("\nUSAGE: ./WebClient [options] <args>\n\n\tThis is the client application.\n\tThe client connects with BrokerageCenter.\n\nOPTIONS");
    desc.add_options() // <--- every line-end from here needs a comment mark so that to prevent auto formating into single line
        (CSTR_HELP ",h", "produce help message") //
        (CSTR_CONFIG ",c", po::value<std::string>(), "set config directory") //
        (CSTR_VERBOSE ",v", "verbose mode that dumps detailed server information") //
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

    if (vm.count(CSTR_VERBOSE)) {
        params.isVerbose = true;
    }

    voh_t voh(cout, params.isVerbose);

    MainClient* pMClient = new MainClient("webclient");
    pMClient->setVerbose(params.isVerbose);
    shift::FIXInitiator::getInstance().connectBrokerageCenter(params.configDir + "initiator.cfg", pMClient, "password", params.isVerbose, 1000);

    // Get all company names and send it to front - Start
    pMClient->requestCompanyNames();
    std::thread tCompanyNames(&MainClient::sendCompanyNamesToFront, pMClient);
    tCompanyNames.detach();
    // Get all company names and send it to front - End

    std::thread tRecReq(&MainClient::receiveRequestFromPHP, pMClient);

    pMClient->subAllOrderBook();

    std::thread(
        [&pMClient] {
            while (!pMClient->subAllCandleData()) {
                std::this_thread::sleep_for(1s);
            }
        })
        .detach();

    pMClient->sendStockListToFront();

    /** @brief start thread of WebClient::checkEverySecond function */
    std::thread tCheck(&MainClient::checkEverySecond, pMClient);

    /** @brief thrift service start */
    int port = 9090;
    apache::thrift::stdcxx::shared_ptr<SHIFTServiceHandler> handler(new SHIFTServiceHandler());
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TProcessor> processor(new SHIFTServiceProcessor(handler));
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TServerTransport> serverTransport(new apache::thrift::transport::TServerSocket(port));
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TTransportFactory> transportFactory(new apache::thrift::transport::TBufferedTransportFactory());
    apache::thrift::stdcxx::shared_ptr<apache::thrift::server::TProtocolFactory> protocolFactory(new apache::thrift::server::TBinaryProtocolFactory());

    apache::thrift::server::TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    std::thread tThrift(&apache::thrift::server::TSimpleServer::serve, &server);

    /** @brief join all threads before exit */
    server.stop();
    tThrift.join();
    tCheck.join();
    tRecReq.join();
    return 0;
}
