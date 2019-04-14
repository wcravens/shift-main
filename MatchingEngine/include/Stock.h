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

    void bufNewGlobalOrder(Order&& newOrder);
    void bufNewLocalOrder(Order&& newOrder);
    bool getNextOrder(Order& orderRef);

    void executeGlobalOrder(Order& orderRef, int size, double price, char decision);
    void executeLocalOrder(Order& orderRef, int size, double price, char decision);

    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);
    void orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);

    void showGlobalOrderBooks();
    void showLocalOrderBooks();

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
