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
#include <quickfix/fix50sp2/Advertisement.h>
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MassQuoteAcknowledgement.h>
#include <quickfix/fix50sp2/PositionReport.h>
#include <quickfix/fix50sp2/SecurityList.h>
#include <quickfix/fix50sp2/SecurityStatus.h>
#include <quickfix/fix50sp2/UserResponse.h>

// Sending Message Types
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/RFQRequest.h>
#include <quickfix/fixt11/Logon.h>

#define R_FIXINIT /**/
#define R_CHECKED /**/
#define R_FIXSUB /**/
#define R_REMOVE /**/
#define R_TODO /**/

namespace shift {

/**
 * @brief FIX Initiator for LibCoreClient to communicate with Brokerage Center
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

    ~FIXInitiator() override;

    static FIXInitiator& getInstance();

    void connectBrokerageCenter(const std::string& cfgFile, CoreClient* client, const std::string& password, bool verbose = false, int timeout = 1000);
    void disconnectBrokerageCenter();

    // Call this function to attach both ways
    R_REMOVE bool attachToClient(shift::CoreClient* client);

    // call this function to send webClient username to and register at BC, with a userID as response, if any, for LC internal use. Returns true if user is resolved in BC.
    bool registerUserInBCWaitResponse(shift::CoreClient* client);

    std::vector<CoreClient*> getAttachedClients();
    R_REMOVE CoreClient* getSuperUser();
    CoreClient* getClient(const std::string& name); // for end-clients compartabiliy use

protected:
    CoreClient* getClientByUserID(const std::string& userID); // for core-client internal use

    R_FIXINIT bool isConnected();

    // Inline methods
    R_TODO void debugDump(const std::string& message);
    R_TODO void createSymbolMap();
    R_TODO void initializePrices();
    R_TODO void initializeOrderBooks();

    // FIXInitiator - QuickFIX methods
    R_FIXINIT void sendOrderBookRequest(const std::string& symbol, bool isSubscribed);
    R_FIXINIT void sendCandleDataRequest(const std::string& symbol, bool isSubscribed);
    R_FIXINIT void submitOrder(const shift::Order&, const std::string& userID = "");

    R_FIXINIT void onCreate(const FIX::SessionID&) override;
    R_FIXINIT void onLogon(const FIX::SessionID&) override;
    R_FIXINIT void onLogout(const FIX::SessionID&) override;
    R_FIXINIT void toAdmin(FIX::Message&, const FIX::SessionID&) override;
    R_FIXINIT void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    R_FIXINIT void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    R_FIXINIT void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    R_FIXINIT void onMessage(const FIX50SP2::SecurityList&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::Advertisement&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::MarketDataIncrementalRefresh&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::SecurityStatus&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::ExecutionReport&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::PositionReport&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::MassQuoteAcknowledgement&, const FIX::SessionID&) override;
    R_FIXINIT void onMessage(const FIX50SP2::UserResponse& message, const FIX::SessionID&) override;

    // Price methods
    R_FIXSUB double getOpenPrice(const std::string& symbol);
    R_FIXSUB double getLastPrice(const std::string& symbol);
    R_FIXSUB int getLastSize(const std::string& symbol);
    R_FIXSUB std::chrono::system_clock::time_point getLastTradeTime();
    // Order book methods
    R_FIXSUB shift::BestPrice getBestPrice(const std::string& symbol);
    R_FIXSUB std::vector<shift::OrderBookEntry> getOrderBook(const std::string& symbol, OrderBook::Type type, int maxLevel);
    R_FIXSUB std::vector<shift::OrderBookEntry> getOrderBookWithDestination(const std::string& symbol, OrderBook::Type type);

    // Symbols list and company names
    R_FIXSUB std::vector<std::string> getStockList();
    R_TODO void fetchCompanyName(std::string tickerName);
    R_FIXSUB void requestCompanyNames();
    R_FIXSUB std::map<std::string, std::string> getCompanyNames();
    R_FIXSUB std::string getCompanyName(const std::string& symbol);

    // Subscription methods
    R_FIXSUB void subOrderBook(const std::string& symbol);
    R_FIXSUB void unsubOrderBook(const std::string& symbol);
    R_FIXSUB void subAllOrderBook();
    R_FIXSUB void unsubAllOrderBook();
    R_FIXSUB std::vector<std::string> getSubscribedOrderBookList();
    R_FIXSUB void subCandleData(const std::string& symbol);
    R_FIXSUB void unsubCandleData(const std::string& symbol);
    R_FIXSUB void subAllCandleData();
    R_FIXSUB void unsubAllCandleData();
    R_FIXSUB std::vector<std::string> getSubscribedCandlestickList();

private:
    FIXInitiator();

    // Do NOT change order of these unique_ptrs:
    R_FIXINIT std::unique_ptr<FIX::LogFactory> m_pLogFactory;
    R_FIXINIT std::unique_ptr<FIX::MessageStoreFactory> m_pMessageStoreFactory;
    R_FIXINIT std::unique_ptr<FIX::SocketInitiator> m_pSocketInitiator;

    // FIXInitiator - Logon related
    R_FIXINIT std::string m_superUsername;
    R_FIXINIT std::string m_superUserPsw;
    R_FIXINIT std::string m_superUserID;
    R_FIXINIT std::atomic<bool> m_connected;
    R_FIXINIT std::atomic<bool> m_verbose;
    R_FIXINIT std::atomic<bool> m_logonSuccess;
    R_FIXINIT std::mutex m_mtxLogon;
    R_FIXINIT std::condition_variable m_cvLogon;

    // FIXInitiator - Subscriber management
    R_FIXINIT std::mutex m_mtxClientByUserID;
    R_FIXINIT std::unordered_map<std::string, CoreClient*> m_clientByUserID;

    R_TODO std::atomic<bool> m_openPricesReady;
    R_FIXSUB std::mutex m_mtxOpenPrices;
    R_FIXSUB std::unordered_map<std::string, double> m_openPrices; //!< Map with stock symbol as key and open price as value
    R_FIXSUB std::unordered_map<std::string, std::pair<double, int>> m_lastTrades; //!< Map with stock symbol as key and a pair with their last price and size as value.
    R_FIXSUB std::chrono::system_clock::time_point m_lastTradeTime;

    R_FIXSUB std::unordered_map<std::string, std::map<OrderBook::Type, shift::OrderBook*>> m_orderBooks; //!< Map for orderbook: key is stock symbol, value is another map with type as key and order book as value.

    std::mutex m_mtxUserIDByUsername;
    std::condition_variable m_cvUserIDByUsername;
    std::unordered_map<std::string, std::string> m_userIDByUsername;

    std::mutex m_mtxStockList;
    std::condition_variable m_cvStockList;
    std::vector<std::string> m_stockList; //!< Vector of string saved names of all requested stock.
    std::unordered_map<std::string, std::string> m_originalName_symbol;
    std::unordered_map<std::string, std::string> m_symbol_originalName;
    R_FIXSUB std::mutex m_mtxCompanyNames;
    R_FIXSUB std::map<std::string, std::string> m_companyNames;

    R_TODO std::mutex m_mtxSubscribedOrderBookSet; //!< Mutex for the subscribed order book list
    R_TODO std::set<std::string> m_subscribedOrderBookSet; //!< Set of stock symbols whose orderbook has been subscribed.
    R_TODO std::mutex m_mtxSubscribedCandleStickSet; //!< Mutex for the subscribed candle stick list
    R_TODO std::set<std::string> m_subscribedCandleStickSet; //!< Set of stock symbols whose candlestick data has been subscribed.
};

} // shift
