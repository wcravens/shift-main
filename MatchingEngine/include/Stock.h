#pragma once

#include "ExecutionReport.h"
#include "Order.h"
#include "OrderBookEntry.h"
#include "PriceLevel.h"

#include <list>
#include <mutex>
#include <queue>
#include <string>

#include <quickfix/FieldTypes.h>

class Stock {
public:
    std::list<ExecutionReport> executionReports;
    std::list<OrderBookEntry> orderBookUpdates;

    Stock() = default;
    Stock(const std::string& symbol);
    Stock(const Stock& other);

    const std::string& getSymbol() const;
    void setSymbol(const std::string& symbol);

    void bufNewGlobalOrder(const Order& order);
    void bufNewLocalOrder(const Order& order);
    bool getNextOrder(Order& nextOrder);

    void executeGlobalOrder(int size, Order& newOrder, char decision, const std::string& destination);
    void executeLocalOrder(int size, Order& newOrder, char decision, const std::string& destination);

    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);
    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);

    void showGlobalOrderBooks();
    void showLocalOrderBooks();

    void doLimitBuy(Order& newOrder, const std::string& destination);
    void doLimitSell(Order& newOrder, const std::string& destination);
    void doMarketBuy(Order& newOrder, const std::string& destination);
    void doMarketSell(Order& newOrder, const std::string& destination);
    void doLocalCancelBid(Order& newOrder, const std::string& destination);
    void doLocalCancelAsk(Order& newOrder, const std::string& destination);

    void updateGlobalBids(const Order& order);
    void updateGlobalAsks(const Order& order);

    void doLocalLimitBuy(Order& newOrder, const std::string& destination);
    void doLocalLimitSell(Order& newOrder, const std::string& destination);
    void doLocalMarketBuy(Order& newOrder, const std::string& destination);
    void doLocalMarketSell(Order& newOrder, const std::string& destination);

    void insertLocalBid(const Order& order);
    void insertLocalAsk(const Order& order);

private:
    std::string m_symbol;
    unsigned int m_depth = 5;

    // buffer new quotes & trades received from DE
    std::mutex m_mtxNewGlobalOrders;
    std::queue<Order> m_newGlobalOrders;

    std::list<Order> m_globalBids;
    std::list<Order> m_globalAsks;

    std::list<Order>::iterator m_thisGlobalOrder;

    // buffer new orders received from clients
    std::queue<Order> m_newLocalOrders;
    std::mutex m_mtxNewLocalOrders;

    std::list<PriceLevel> m_localBids;
    std::list<PriceLevel> m_localAsks;

    std::list<PriceLevel>::iterator m_thisPriceLevel;
    std::list<Order>::iterator m_thisLocalOrder;
};
