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
 * @brief Core class for LibCoreClient including all core functions.
 */
class CORECLIENT_EXPORTS FIXInitiator;

class CORECLIENT_EXPORTS CoreClient {

    //! The source of all evils
    friend class FIXInitiator;

public:
    CoreClient();
    CoreClient(std::string username);
    virtual ~CoreClient() = default;

    void setVerbose(bool verbose);
    auto isVerbose() const -> bool;
    auto isConnected() const -> bool;

    auto getUsername() const -> const std::string&;
    void setUsername(const std::string& username);
    auto getUserID() const -> const std::string&;
    void setUserID(const std::string& userID);
    auto getAttachedClients() -> std::vector<CoreClient*>;

    void submitOrder(const Order& order);
    void submitCancellation(Order order);

    // portfolio methods
    auto getPortfolioSummary() -> PortfolioSummary;
    auto getPortfolioItems() -> std::map<std::string, PortfolioItem>;
    auto getPortfolioItem(const std::string& symbol) -> PortfolioItem;
    auto getUnrealizedPL(const std::string& symbol = "") -> double;
    auto getSubmittedOrdersSize() -> int;
    auto getSubmittedOrders() -> std::vector<Order>;
    auto getOrder(const std::string& orderID) -> Order;
    auto getExecutedOrders(const std::string& orderID) -> std::vector<Order>;
    auto getWaitingListSize() -> int;
    auto getWaitingList() -> std::vector<Order>;
    void cancelAllPendingOrders(int timeout = 10);

    // price methods
    auto getOpenPrice(const std::string& symbol) -> double;
    auto getClosePrice(const std::string& symbol, bool buy, int size) -> double;
    auto getClosePrice(const std::string& symbol) -> double;
    auto getLastPrice(const std::string& symbol) -> double;
    auto getLastSize(const std::string& symbol) -> int;
    auto getLastTradeTime() -> std::chrono::system_clock::time_point;

    // order book methods
    auto getBestPrice(const std::string& symbol) -> BestPrice;
    auto getOrderBook(const std::string& symbol, const OrderBook::Type& type, int maxLevel = 99) -> std::vector<OrderBookEntry>;
    auto getOrderBookWithDestination(const std::string& symbol, const OrderBook::Type& type) -> std::vector<OrderBookEntry>;

    // symbols list and company names
    auto getStockList() -> std::vector<std::string>;
    void requestCompanyNames();
    auto getCompanyNames() -> std::map<std::string, std::string>;
    auto getCompanyName(const std::string& symbol) -> std::string;

    // sample prices
    auto requestSamplePrices(std::vector<std::string> symbols, double samplingFrequencyS = 1.0, int samplingWindow = 31) -> bool;
    auto cancelSamplePricesRequest(const std::vector<std::string>& symbols) -> bool;
    auto cancelAllSamplePricesRequests() -> bool;
    auto getSamplePricesSize(const std::string& symbol) -> int;
    auto getSamplePrices(const std::string& symbol, bool midPrices = false) -> std::list<double>;
    auto getLogReturnsSize(const std::string& symbol) -> int;
    auto getLogReturns(const std::string& symbol, bool midPrices = false) -> std::list<double>;

    // subscription methods
    auto subOrderBook(const std::string& symbol) -> bool;
    auto unsubOrderBook(const std::string& symbol) -> bool;
    auto subAllOrderBook() -> bool;
    auto unsubAllOrderBook() -> bool;
    auto getSubscribedOrderBookList() -> std::vector<std::string>;
    auto subCandlestickData(const std::string& symbol) -> bool;
    auto unsubCandlestickData(const std::string& symbol) -> bool;
    auto subAllCandlestickData() -> bool;
    auto unsubAllCandlestickData() -> bool;
    auto getSubscribedCandlestickDataList() -> std::vector<std::string>;

protected:
    // inline methods
    void debugDump(const std::string& message) const;

    // FIXInitiator interface
    bool attachInitiator(FIXInitiator& initiator);
    void storeExecution(const std::string& orderID, Order::Type orderType, int executedSize, double executedPrice, Order::Status newStatus);
    void storePortfolioSummary(double totalBP, int totalShares, double totalRealizedPL);
    void storePortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL);
    void storeWaitingList(std::vector<Order>&& waitingList);

    // FIXInitiator callback methods
    virtual void receiveLastPrice(const std::string& symbol) {}
    virtual void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) {}
    virtual void receiveExecution(const std::string& orderID) {}
    virtual void receivePortfolioSummary() {}
    virtual void receivePortfolioItem(const std::string& symbol) {}
    virtual void receiveWaitingList() {}

    // sample prices
    void calculateSamplePrices(std::vector<std::string> symbols, double samplingFrequencyS, int samplingWindow);

private:
    FIXInitiator* m_fixInitiator;
    std::string m_username;
    std::string m_userID;
    bool m_verbose;

    mutable std::mutex m_mutex_portfolioSummary;
    mutable std::mutex m_mutex_symbol_portfolioItem;
    mutable std::mutex m_mutex_orders;
    mutable std::mutex m_mutex_waitingList;
    mutable std::mutex m_mutex_samplePricesFlags;
    mutable std::mutex m_mutex_samplePrices;

    PortfolioSummary m_portfolioSummary;
    std::map<std::string, PortfolioItem> m_symbol_portfolioItem;
    std::vector<std::string> m_submittedOrdersIDs;
    std::unordered_map<std::string, Order> m_submittedOrders;
    std::atomic<int> m_submittedOrdersSize;
    std::unordered_multimap<std::string, Order> m_executedOrders;
    std::vector<Order> m_waitingList;
    std::atomic<int> m_waitingListSize;

    std::vector<std::thread> m_samplePriceThreads;
    std::unordered_map<std::string, bool> m_samplePricesFlags;
    std::unordered_map<std::string, std::list<double>> m_sampleLastPrices;
    std::unordered_map<std::string, std::list<double>> m_sampleMidPrices;
};

} // shift
