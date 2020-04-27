#pragma once

#include "ExecutionReport.h"
#include "FIXAcceptor.h"
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

namespace markets {

class StockMarket {
public:
    StockMarket() = default;
    StockMarket(std::string symbol);
    StockMarket(const StockMarket& other);
    virtual ~StockMarket() = default;

    auto getSymbol() const -> const std::string&;
    void setSymbol(const std::string& symbol);

    void displayGlobalOrderBooks();
    void displayLocalOrderBooks();

    void sendOrderBookDataToTarget(const std::string& targetID);

    void bufNewGlobalOrder(Order&& newOrder);
    void bufNewLocalOrder(Order&& newOrder);

    // function to start one stock matching engine, for stock market thread
    virtual void operator()() = 0;

protected:
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

    auto getNextOrder(Order& orderRef) -> bool;

    void executeGlobalOrder(Order& orderRef, int size, double price, char decision);
    void executeLocalOrder(Order& orderRef, int size, double price, char decision);

    inline void addExecutionReport(ExecutionReport&& newExecutionReport)
    {
        m_executionReports.push_back(std::move(newExecutionReport));
    }

    inline void sendExecutionReports()
    {
        FIXAcceptor::getInstance().sendExecutionReports(m_executionReports);
        m_executionReports.clear();
    }

    inline void addOrderBookUpdate(OrderBookEntry&& newOrderBookUpdate)
    {
        m_orderBookUpdates.push_back(std::move(newOrderBookUpdate));
    }

    inline void sendOrderBookUpdates()
    {
        FIXAcceptor::getInstance().sendOrderBookUpdates(m_orderBookUpdates);
        m_orderBookUpdates.clear();
    };

private:
    // vectors to hold temporary information
    std::vector<ExecutionReport> m_executionReports;
    std::vector<OrderBookEntry> m_orderBookUpdates;
};

class StockMarketList {
public:
    using stock_market_list_t = std::map<std::string, std::unique_ptr<StockMarket>>;

    static std::atomic<bool> s_isTimeout;

    static auto getInstance() -> stock_market_list_t&;

private:
    StockMarketList() = default;
};

} // markets
