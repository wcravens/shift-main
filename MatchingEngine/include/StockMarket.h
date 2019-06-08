#pragma once

#include "ExecutionReport.h"
#include "Order.h"
#include "OrderBookEntry.h"
#include "PriceLevel.h"

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <quickfix/FieldTypes.h>

class StockMarket {
public:
    std::vector<ExecutionReport> executionReports;
    std::vector<OrderBookEntry> orderBookUpdates;

    StockMarket() = default;
    StockMarket(const std::string& symbol);
    StockMarket(const StockMarket& other);

    // Function to start one stock matching engine, for exchange thread
    void operator()();

    const std::string& getSymbol() const;
    void setSymbol(const std::string& symbol);

    void sendOrderBookDataToTarget(const std::string& targetID);

    void bufNewGlobalOrder(Order&& newOrder);
    void bufNewLocalOrder(Order&& newOrder);
    bool getNextOrder(Order& orderRef);

    void executeGlobalOrder(Order& orderRef, int size, double price, char decision);
    void executeLocalOrder(Order& orderRef, int size, double price, char decision);

    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& simulationTime);
    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& simulationTime);

    void displayGlobalOrderBooks();
    void displayLocalOrderBooks();

    void doGlobalLimitBuy(Order& orderRef);
    void doGlobalLimitSell(Order& orderRef);

    void updateGlobalBids(Order newBestBid);
    void updateGlobalAsks(Order newBestAsk);

    void doLocalLimitBuy(Order& orderRef);
    void doLocalLimitSell(Order& orderRef);
    void doLocalMarketBuy(Order& orderRef);
    void doLocalMarketSell(Order& orderRef);
    void doLocalCancelBid(Order& orderRef);
    void doLocalCancelAsk(Order& orderRef);

    void insertLocalBid(Order newBid);
    void insertLocalAsk(Order newAsk);

private:
    std::string m_symbol;
    std::atomic_flag m_spinlock = ATOMIC_FLAG_INIT;

    // buffer new quotes & trades received from DE
    std::mutex m_mtxNewGlobalOrders;
    std::queue<Order> m_newGlobalOrders;

    std::list<Order> m_globalBids;
    std::list<Order> m_globalAsks;

    std::list<Order>::iterator m_thisGlobalOrder;

    // buffer new orders received from clients
    std::mutex m_mtxNewLocalOrders;
    std::queue<Order> m_newLocalOrders;

    std::list<PriceLevel> m_localBids;
    std::list<PriceLevel> m_localAsks;

    std::list<PriceLevel>::iterator m_thisPriceLevel;
    std::list<Order>::iterator m_thisLocalOrder;
};

class StockMarketList {
public:
    using stock_market_list_t = std::map<std::string, StockMarket>;

    static std::atomic<bool> s_isTimeout;

    static stock_market_list_t& getInstance();

private:
    StockMarketList() = default;
};
