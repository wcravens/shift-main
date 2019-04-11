#pragma once

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#include "BestPrice.h"
#include "CoreClient_EXPORTS.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <quickfix/Message.h>

namespace shift {
/**
 * @brief Core class for LibCoreClient including all core functions
 */
class CORECLIENT_EXPORTS FIXInitiator;
class CORECLIENT_EXPORTS CoreClient {

    friend class FIXInitiator;

public:
    CoreClient();
    CoreClient(const std::string& username);
    virtual ~CoreClient();

    void setVerbose(bool verbose);
    bool isVerbose();
    bool isConnected();

    std::string getUsername() const;
    void setUsername(const std::string& username);
    std::string getUserID() const;
    void setUserID(const std::string& userID);
    std::vector<CoreClient*> getAttachedClients();

    void submitOrder(const shift::Order& order);
    void submitCancellation(shift::Order order);

    // Portfolio methods
    PortfolioSummary getPortfolioSummary();
    std::map<std::string, PortfolioItem> getPortfolioItems();
    PortfolioItem getPortfolioItem(const std::string& symbol);
    int getSubmittedOrdersSize();
    std::vector<shift::Order> getSubmittedOrders();
    shift::Order getOrder(const std::string& orderID);
    int getWaitingListSize();
    std::vector<shift::Order> getWaitingList();
    void cancelAllPendingOrders();

    // Price methods
    double getOpenPrice(const std::string& symbol);
    double getClosePrice(const std::string& symbol, bool buy, int size);
    double getLastPrice(const std::string& symbol);
    int getLastSize(const std::string& symbol);
    std::chrono::system_clock::time_point getLastTradeTime();

    // Order book methods
    shift::BestPrice getBestPrice(const std::string& symbol);
    std::vector<shift::OrderBookEntry> getOrderBook(const std::string& symbol, const OrderBook::Type& type, int maxLevel = 99);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestination(const std::string& symbol, const OrderBook::Type& type);

    // Symbols list and company names
    std::vector<std::string> getStockList();
    void requestCompanyNames();
    std::map<std::string, std::string> getCompanyNames();
    std::string getCompanyName(const std::string& symbol);

    // Sample prices
    bool requestSamplePrices(std::vector<std::string> symbols, double samplingFrequency = 1, unsigned int samplingWindow = 31);
    bool cancelSamplePricesRequest(const std::vector<std::string>& symbols);
    bool cancelAllSamplePricesRequests();
    int getSamplePricesSize(const std::string& symbol);
    std::list<double> getSamplePrices(const std::string& symbol, bool midPrices = false);
    int getLogReturnsSize(const std::string& symbol);
    std::list<double> getLogReturns(const std::string& symbol, bool midPrices = false);

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

protected:
    // Inline methods
    void debugDump(const std::string& message);

    // FIXInitiator interface
    bool attachInitiator(FIXInitiator& initiator);
    void storeExecution(const std::string& orderID, int executedSize, double executedPrice, shift::Order::Status newStatus);
    void storePortfolioSummary(double totalBP, int totalShares, double totalRealizedPL);
    void storePortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL);
    void storeWaitingList(std::vector<shift::Order>&& waitingList);

    // FIXInitiator callback methods
    virtual void receiveLastPrice(const std::string& /*symbol*/) {}
    virtual void receiveCandlestickData(const std::string& /*symbol*/, double /*open*/, double /*high*/, double /*low*/, double /*close*/, const std::string& /*timestamp*/) {}
    virtual void receiveExecution(const std::string& /*orderID*/) {}
    virtual void receivePortfolioSummary() {}
    virtual void receivePortfolioItem(const std::string& /*symbol*/) {}
    virtual void receiveWaitingList() {}

    // Sample prices
    void calculateSamplePrices(std::vector<std::string> symbols, double samplingFrequency, unsigned int samplingWindow);

private:
    FIXInitiator* m_fixInitiator;
    std::string m_username;
    std::string m_userID;
    bool m_verbose;

    std::mutex m_mutex_portfolioSummary;
    std::mutex m_mutex_symbol_portfolioItem; //!< Mutex for the <Symbol, PortfolioItem> map.
    std::mutex m_mutex_submittedOrders;
    std::mutex m_mutex_waitingList;
    std::mutex m_mutex_samplePricesFlags;
    std::mutex m_mutex_samplePrices;

    PortfolioSummary m_portfolioSummary;
    std::map<std::string, PortfolioItem> m_symbol_portfolioItem;
    std::vector<std::string> m_submittedOrdersIDs;
    std::unordered_map<std::string, shift::Order> m_submittedOrders;
    std::atomic<int> m_submittedOrdersSize;

    std::vector<shift::Order> m_waitingList;
    std::atomic<int> m_waitingListSize;
    std::vector<std::thread> m_samplePriceThreads;
    std::unordered_map<std::string, bool> m_samplePricesFlags;
    std::unordered_map<std::string, std::list<double>> m_sampleLastPrices;
    std::unordered_map<std::string, std::list<double>> m_sampleMidPrices;
};

} // shift
