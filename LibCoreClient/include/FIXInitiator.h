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

#define R_FIXINIT /**/
#define R_CHECKED /**/
#define R_FIXSUB /**/
#define R_TODO /**/

namespace shift {

/** 
 * @brief FIX Initiator for LibCoreClient to communicate with Brokerage Center
 */
class CORECLIENT_EXPORTS CoreClient;
class CORECLIENT_EXPORTS FIXInitiator
        : public FIX::Application
        , public FIX::MessageCracker {

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
    R_FIXINIT bool isConnected();

    // Inline methods
    R_TODO void debugDump(const std::string& message);
    R_TODO void createSymbolMap();
    R_TODO void initializePrices();
    R_TODO void initializeOrderBooks();

    // FIXInitiator - QuickFIX methods
    R_FIXINIT void onCreate(const FIX::SessionID& sessionID) override;
    R_FIXINIT void onLogon(const FIX::SessionID& sessionID) override;
    R_FIXINIT void onLogout(const FIX::SessionID& sessionID) override;
    R_FIXINIT void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    R_FIXINIT void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    R_FIXINIT void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    R_FIXINIT void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    R_FIXINIT void onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) override;
    R_FIXINIT void onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID& sessionID) override; // for testing when receiving order book
    R_FIXINIT void onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID& sessionID) override;
    R_FIXINIT void onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) override;
    R_FIXINIT void onMessage(const FIX50SP2::SecurityStatus& message, const FIX::SessionID& sessionID) override;

    R_FIXINIT void sendCandleDataRequest(const std::string& symbol, bool isSubscribed);
    R_FIXINIT void sendOrderBookRequest(const std::string& symbol, bool isSubscribed);
    R_FIXINIT void submitOrder(const shift::Order&, const std::string& username = "");

    // Price methods
    R_FIXSUB double getOpenPriceBySymbol(const std::string& symbol);
    R_FIXSUB double getLastPriceBySymbol(const std::string& symbol);
    // Order book methods
    R_FIXSUB shift::BestPrice getBestPriceBySymbol(const std::string& symbol);
    R_FIXSUB std::vector<shift::OrderBookEntry> getOrderBook(const std::string& symbol, OrderBook::Type type);
    R_FIXSUB std::vector<shift::OrderBookEntry> getOrderBookWithDestination(const std::string& symbol, OrderBook::Type type);

    // Symbols list and company names
    R_FIXSUB std::vector<std::string> getStockList();
    R_TODO void fetchCompanyName(const std::string tickerName);
    R_FIXSUB void requestCompanyNames();
    R_FIXSUB std::map<std::string, std::string> getCompanyNames();
    R_FIXSUB std::string getCompanyNameBySymbol(const std::string& symbol);

    // Subscription methods
    R_FIXSUB bool subOrderBook(const std::string& symbol);
    R_FIXSUB bool unsubOrderBook(const std::string& symbol);
    R_FIXSUB bool subAllOrderBook();
    R_FIXSUB bool unsubAllOrderBook();
    R_FIXSUB std::vector<std::string> getSubscribedOrderBookList();
    R_FIXSUB bool subCandleData(const std::string& symbol);
    R_FIXSUB bool unsubCandleData(const std::string& symbol);
    R_FIXSUB bool subAllCandleData();
    R_FIXSUB bool unsubAllCandleData();
    R_FIXSUB std::vector<std::string> getSubscribedCandlestickList();

private:
    FIXInitiator();


    // Do NOT change order of these unique_ptrs:
    R_FIXINIT std::unique_ptr<FIX::LogFactory> m_pLogFactory;
    R_FIXINIT std::unique_ptr<FIX::MessageStoreFactory> m_pMessageStoreFactory;
    R_FIXINIT std::unique_ptr<FIX::SocketInitiator> m_pSocketInitiator;

    // FIXInitiator - Logon related
    R_FIXINIT std::string m_username;
    R_FIXINIT std::string m_password;
    R_FIXINIT std::atomic<bool> m_connected;
    R_FIXINIT std::atomic<bool> m_verbose;
    R_FIXINIT std::atomic<bool> m_logonSuccess;
    R_FIXINIT std::mutex m_mutex_logon;
    R_FIXINIT std::condition_variable m_cv_logon;

    // FIXInitiator - Subscriber management
    R_FIXINIT std::unordered_map<std::string, CoreClient*> m_username_client;



    R_TODO std::atomic<bool> m_openPricesReady;
    R_TODO std::mutex m_mutex_openPrices;
    R_FIXSUB std::unordered_map<std::string, double> m_openPrices; //!< Map with stock symbol as key and open price as value
    R_FIXSUB std::unordered_map<std::string, double> m_lastPrices; //!< Map with stock symbol as key and their current price as value.

    R_FIXSUB std::unordered_map<std::string, std::map<OrderBook::Type, shift::OrderBook*>> m_orderBooks; //!< Map for orderbook: key is stock symbol, value is another map with type as key and order book as value.

    std::mutex m_mutex_stockList;
    std::condition_variable m_cv_stockList;
    std::vector<std::string> m_stockList; //!< Vector of string saved names of all requested stock.
    std::unordered_map<std::string, std::string> m_symbol_originalName;
    std::unordered_map<std::string, std::string> m_originalName_symbol;
    R_FIXSUB std::mutex m_mutex_companyNames;
    R_FIXSUB std::map<std::string, std::string> m_companyNames;

    R_TODO std::mutex m_mutex_subscribedOrderBookSet; //!< Mutex for the subscribed order book list
    R_TODO std::set<std::string> m_subscribedOrderBookSet; //!< Set of stock symbols whose orderbook has been subscribed.
    R_TODO std::mutex m_mutex_subscribedCandleStickSet; //!< Mutex for the subscribed candle stick list
    R_TODO std::set<std::string> m_subscribedCandleStickSet; //!< Set of stock symbols whose candlestick data has been subscribed.
};

} // shift
