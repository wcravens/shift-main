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

    void executeGlobalOrder(int size, Order& orderRef, char decision);
    void executeLocalOrder(int size, Order& orderRef, char decision);

    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);
    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);

    void showGlobalOrderBooks();
    void showLocalOrderBooks();

    void doLimitBuy(Order& orderRef);
    void doLimitSell(Order& orderRef);
    void doMarketBuy(Order& orderRef);
    void doMarketSell(Order& orderRef);

    void updateGlobalBids(const Order& order);
    void updateGlobalAsks(const Order& order);

    void doLocalLimitBuy(Order& orderRef);
    void doLocalLimitSell(Order& orderRef);
    void doLocalMarketBuy(Order& orderRef);
    void doLocalMarketSell(Order& orderRef);
    void doLocalCancelBid(Order& orderRef);
    void doLocalCancelAsk(Order& orderRef);

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
