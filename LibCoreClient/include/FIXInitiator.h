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

// initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
// #include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

// receiving message types
#include <quickfix/fix50sp2/Advertisement.h>
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/NewOrderList.h>
#include <quickfix/fix50sp2/PositionReport.h>
#include <quickfix/fix50sp2/SecurityList.h>
#include <quickfix/fix50sp2/SecurityStatus.h>
#include <quickfix/fix50sp2/UserResponse.h>

// sending message types
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/RFQRequest.h>
#include <quickfix/fixt11/Logon.h>

namespace shift {

/**
 * @brief FIX Initiator for LibCoreClient to communicate with Brokerage Center.
 */
class CORECLIENT_EXPORTS CoreClient;
class CORECLIENT_EXPORTS FIXInitiator
    : public FIX::Application,
      public FIX::MessageCracker {

    //! The source of all evils
    friend class CoreClient;

public:
    static std::string s_senderID;
    static std::string s_targetID;

    static std::chrono::system_clock::time_point s_convertToTimePoint(const FIX::UtcDateOnly& date, const FIX::UtcTimeOnly& time);

    static FIXInitiator& getInstance();

    bool connectBrokerageCenter(const std::string& configFile, CoreClient* client, const std::string& password, bool verbose = false, int timeout = 10);
    void disconnectBrokerageCenter();

    // call this function to attach both ways
    bool attachClient(shift::CoreClient* client, const std::string& password = "NA");

    // call this function to send webClient username to and register at BC, with a userID as response, if any, for LC internal use
    // returns true if user is resolved in BC
    bool registerUserInBCWaitResponse(shift::CoreClient* client);

    std::vector<CoreClient*> getAttachedClients();
    CoreClient* getSuperUser();
    CoreClient* getClient(const std::string& name); // for end-clients compatibility use

protected:
    CoreClient* getClientByUserID(const std::string& userID); // for core-client internal use

    bool isConnected();

    // inline methods
    void debugDump(const std::string& message);
    void createSymbolMap();
    void initializePrices();
    void initializeOrderBooks();

    // FIXInitiator - QuickFIX methods
    void sendOrderBookRequest(const std::string& symbol, bool isSubscribed);
    void sendCandleDataRequest(const std::string& symbol, bool isSubscribed);
    void submitOrder(const shift::Order&, const std::string& userID = "");

    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override;
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::SecurityList&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::UserResponse&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::Advertisement&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::MarketDataIncrementalRefresh&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::SecurityStatus&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::ExecutionReport&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::PositionReport&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::NewOrderList&, const FIX::SessionID&) override;

    // price methods
    double getOpenPrice(const std::string& symbol);
    double getLastPrice(const std::string& symbol);
    int getLastSize(const std::string& symbol);
    std::chrono::system_clock::time_point getLastTradeTime();

    // order book methods
    shift::BestPrice getBestPrice(const std::string& symbol);
    std::vector<shift::OrderBookEntry> getOrderBook(const std::string& symbol, OrderBook::Type type, int maxLevel);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestination(const std::string& symbol, OrderBook::Type type);

    // symbols list and company names
    std::vector<std::string> getStockList();
    void fetchCompanyName(std::string tickerName);
    void requestCompanyNames();
    std::map<std::string, std::string> getCompanyNames();
    std::string getCompanyName(const std::string& symbol);

    // subscription methods
    void subOrderBook(const std::string& symbol);
    void unsubOrderBook(const std::string& symbol);
    void subAllOrderBook();
    void unsubAllOrderBook();
    std::vector<std::string> getSubscribedOrderBookList();
    void subCandleData(const std::string& symbol);
    void unsubCandleData(const std::string& symbol);
    void subAllCandleData();
    void unsubAllCandleData();
    std::vector<std::string> getSubscribedCandlestickList();

private:
    FIXInitiator();
    ~FIXInitiator() override;

    FIXInitiator(const FIXInitiator&) = delete;
    void operator=(const FIXInitiator&) = delete;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_pLogFactory;
    std::unique_ptr<FIX::MessageStoreFactory> m_pMessageStoreFactory;
    std::unique_ptr<FIX::SocketInitiator> m_pSocketInitiator;

    // FIXInitiator - logon related
    std::string m_superUsername;
    std::string m_superUserPsw;
    std::string m_superUserID;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_verbose;
    std::atomic<bool> m_logonSuccess;
    std::mutex m_mtxLogon;
    std::condition_variable m_cvLogon;

    // FIXInitiator - subscriber management
    std::mutex m_mtxClientByUserID;
    std::unordered_map<std::string, CoreClient*> m_clientByUserID;

    std::atomic<bool> m_openPricesReady;
    std::mutex m_mtxOpenPrices;
    std::unordered_map<std::string, double> m_openPrices; //!< Map with stock symbol as key and open price as value.
    std::unordered_map<std::string, std::pair<double, int>> m_lastTrades; //!< Map with stock symbol as key and a pair with their last price and size as value.
    std::chrono::system_clock::time_point m_lastTradeTime;

    std::unordered_map<std::string, std::map<OrderBook::Type, shift::OrderBook*>> m_orderBooks; //!< Map for orderbook: key is stock symbol, value is another map with type as key and order book as value.

    std::mutex m_mtxUserIDByUsername;
    std::condition_variable m_cvUserIDByUsername;
    std::unordered_map<std::string, std::string> m_userIDByUsername;

    std::mutex m_mtxStockList;
    std::condition_variable m_cvStockList;
    std::vector<std::string> m_stockList; //!< Vector of string saved names of all requested stock.
    std::unordered_map<std::string, std::string> m_originalName_symbol;
    std::unordered_map<std::string, std::string> m_symbol_originalName;
    std::mutex m_mtxCompanyNames;
    std::map<std::string, std::string> m_companyNames;

    std::mutex m_mtxSubscribedOrderBookSet; //!< Mutex for the subscribed order book list.
    std::set<std::string> m_subscribedOrderBookSet; //!< Set of stock symbols whose orderbook has been subscribed.
    std::mutex m_mtxSubscribedCandleStickSet; //!< Mutex for the subscribed candle stick list.
    std::set<std::string> m_subscribedCandleStickSet; //!< Set of stock symbols whose candlestick data has been subscribed.
};

} // shift
