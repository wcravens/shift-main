#pragma once

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#include "BestPrice.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

// Receiving Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MassQuoteAcknowledgement.h>
#include <quickfix/fix50sp2/SecurityList.h>
#include <quickfix/fix50sp2/SecurityStatus.h>

// Sending Message Types
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/RFQRequest.h>
#include <quickfix/fixt11/Logon.h>

namespace shift {

/** 
 * @brief FIX Initiator for LibCoreClient to communicate with Brokerage Center
 */
class CORECLIENT_EXPORTS CoreClient;
class CORECLIENT_EXPORTS FIXInitiator : public FIX::Application, public FIX::MessageCracker {

    friend class CoreClient;

public:
    static std::string s_senderID;
    static std::string s_targetID;

    ~FIXInitiator() override;

    static FIXInitiator& getInstance();

    void connectBrokerageCenter(const std::string& cfgFile, CoreClient* pmc, const std::string& password, bool verbose = false, int timeout = 1000);
    void disconnectBrokerageCenter();

    // Call this function to attach both ways
    void attach(shift::CoreClient* pcc, const std::string& password = "NA", int timeout = 0);

    // call this function to send webClient username to and register at BC
    void webClientSendUsername(const std::string& username);

    std::vector<CoreClient*> getAttachedClients();
    CoreClient* getMainClient();
    CoreClient* getClientByName(const std::string& name);

protected:
    bool isConnected();

    // Inline methods
    void convertToUpperCase(std::string& str);
    void debugDump(const std::string& message);
    void createSymbolMap();
    void initializePrices();
    void initializeOrderBooks();

    // QuickFIX methods
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID& sessionID) override; // for testing when receiving order book
    void onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID& sessionID) override;

    void sendCandleDataRequest(const std::string& symbol, bool isSubscribed);
    void sendOrderBookRequest(const std::string& symbol, bool isSubscribed);
    void submitOrder(const shift::Order&, const std::string& username = "");

    // Price methods
    double getOpenPriceBySymbol(const std::string& symbol);
    double getLastPriceBySymbol(const std::string& symbol);

    // Order book methods
    shift::BestPrice getBestPriceBySymbol(const std::string& symbol);
    std::vector<shift::OrderBookEntry> getOrderBookBySymbolAndType(const std::string& symbol, char type);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestBySymbolAndType(const std::string& symbol, char type);

    // Symbols list and company names
    std::vector<std::string> getStockList();
    static int s_writer(char* data, size_t size, size_t nmemb, std::string* buffer);
    void fetchCompanyName(const std::string tickerName);
    void requestCompanyNames();
    std::map<std::string, std::string> getCompanyNames();
    std::string getCompanyNameBySymbol(const std::string& symbol);

    // Subscription methods
    bool subOrderBook(const std::string& symbol);
    bool unsubOrderBook(const std::string& symbol);
    bool subAllOrderBook();
    bool unsubAllOrderBook();
    std::vector<std::string> getSubscribedOrderBookList();
    bool subCandleData(const std::string& symbol);
    bool unsubCandleData(const std::string& symbol);
    bool subAllCandleData();
    bool unsubAllCandleData();
    std::vector<std::string> getSubscribedCandlestickList();

private:
    FIXInitiator();

    std::atomic<bool> m_connected;
    std::atomic<bool> m_verbose;

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_pLogFactory;
    std::unique_ptr<FIX::MessageStoreFactory> m_pMessageStoreFactory;
    std::unique_ptr<FIX::SocketInitiator> m_pSocketInitiator;

    std::atomic<bool> m_logonSuccess;
    std::mutex m_mutex_logon;
    std::condition_variable m_cv_logon;

    // Username and password to login to BC (as FIXInitiator)
    std::string m_username;
    std::string m_password;

    std::unordered_map<std::string, CoreClient*> m_username_client;

    std::atomic<bool> m_openPricesReady;
    std::mutex m_mutex_openPrices;
    std::unordered_map<std::string, double> m_openPrices; //!< Map with stock symbol as key and open price as value
    std::unordered_map<std::string, double> m_lastPrices; //!< Map with stock symbol as key and their current price as value.

    std::unordered_map<std::string, std::map<char, shift::OrderBook*>> m_orderBooks; //!< Map for orderbook: key is stock symbol, value is another map with type as key and order book as value.

    std::mutex m_mutex_stockList;
    std::condition_variable m_cv_stockList;
    std::mutex m_mutex_companyNames;
    std::vector<std::string> m_stockList; //!< Vector of string saved names of all requested stock.
    std::unordered_map<std::string, std::string> m_symbol_originalName;
    std::unordered_map<std::string, std::string> m_originalName_symbol;
    std::map<std::string, std::string> m_companyNames;

    std::mutex m_mutex_subscribedOrderBookSet; //!< Mutex for the subscribed order book list
    std::mutex m_mutex_subscribedCandleStickSet; //!< Mutex for the subscribed candle stick list
    std::set<std::string> m_subscribedOrderBookSet; //!< Set of stock symbols whose orderbook has been subscribed.
    std::set<std::string> m_subscribedCandleStickSet; //!< Set of stock symbols whose candlestick data has been subscribed.
};

} // shift
