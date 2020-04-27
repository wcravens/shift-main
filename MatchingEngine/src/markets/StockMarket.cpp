#include "markets/StockMarket.h"

#include "FIXAcceptor.h"
#include "TimeSetting.h"

#include <chrono>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

namespace markets {

StockMarket::StockMarket(std::string symbol)
    : m_symbol { std::move(symbol) }
{
}

StockMarket::StockMarket(const StockMarket& other)
    : m_symbol { other.m_symbol }
{
}

auto StockMarket::getSymbol() const -> const std::string&
{
    return m_symbol;
}

void StockMarket::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void StockMarket::displayGlobalOrderBooks()
{
    long simulationMilli = TimeSetting::getInstance().pastMilli(true);

    cout << endl
         << "(" << simulationMilli << ") "
         << "Global Ask:"
         << endl;

    for (const auto& globalAsk : m_globalAsks) {
        cout << globalAsk.getPrice() << '\t' << globalAsk.getSize() << '\t' << globalAsk.getDestination() << endl;
    }

    cout << endl
         << "(" << simulationMilli << ") "
         << "Global Bid:"
         << endl;

    for (const auto& globalBid : m_globalBids) {
        cout << globalBid.getPrice() << '\t' << globalBid.getSize() << '\t' << globalBid.getDestination() << endl;
    }

    cout << endl;
}

void StockMarket::displayLocalOrderBooks()
{
    long simulationMilli = TimeSetting::getInstance().pastMilli(true);

    cout << endl
         << "(" << simulationMilli << ") "
         << "Local Ask:"
         << endl;

    for (const auto& localAsk : m_localAsks) {
        cout << localAsk.getPrice() << '\t' << localAsk.getSize() << endl;
    }

    cout << endl
         << "(" << simulationMilli << ") "
         << "Local Bid:"
         << endl;

    for (const auto& localBid : m_localBids) {
        cout << localBid.getPrice() << '\t' << localBid.getSize() << endl;
    }

    cout << endl;
}

void StockMarket::sendOrderBookDataToTarget(const std::string& targetID)
{
    // temporary vectors to hold order book entries
    std::vector<OrderBookEntry> globalBids;
    std::vector<OrderBookEntry> globalAsks;
    std::vector<OrderBookEntry> localBids;
    std::vector<OrderBookEntry> localAsks;

    // spinlock implementation:
    // it is better than a standard lock in this scenario, since
    // this function will only be executed a couple of times during a simulation
    while (m_spinlock.test_and_set()) {
    }

    // requirements:
    // - the first order book entry must have a price <= 0.0. This will signal the BC it
    // needs to clear its current version of the order book before accepting new entries
    // - order book entries are sent from worst to best price (in reverse order) so
    // that the BC can use the same update procedure as normal order book updates

    auto now = TimeSetting::getInstance().simulationTimestamp();

    globalBids.emplace_back(OrderBookEntry::Type::GLB_BID, m_symbol, 0.0, 0, now);
    for (auto rit = m_globalBids.rbegin(); rit != m_globalBids.rend(); ++rit) {
        globalBids.emplace_back(OrderBookEntry::Type::GLB_BID, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now);
    }

    globalAsks.emplace_back(OrderBookEntry::Type::GLB_ASK, m_symbol, 0.0, 0, now);
    for (auto rit = m_globalAsks.rbegin(); rit != m_globalAsks.rend(); ++rit) {
        globalAsks.emplace_back(OrderBookEntry::Type::GLB_ASK, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now);
    }

    localBids.emplace_back(OrderBookEntry::Type::LOC_BID, m_symbol, 0.0, 0, now);
    for (auto rit = m_localBids.rbegin(); rit != m_localBids.rend(); ++rit) {
        localBids.emplace_back(OrderBookEntry::Type::LOC_BID, m_symbol, rit->getPrice(), rit->getSize(), now);
    }

    localAsks.emplace_back(OrderBookEntry::Type::LOC_ASK, m_symbol, 0.0, 0, now);
    for (auto rit = m_localAsks.rbegin(); rit != m_localAsks.rend(); ++rit) {
        localAsks.emplace_back(OrderBookEntry::Type::LOC_ASK, m_symbol, rit->getPrice(), rit->getSize(), now);
    }

    FIXAcceptor::s_sendOrderBook(targetID, globalBids);
    FIXAcceptor::s_sendOrderBook(targetID, globalAsks);
    FIXAcceptor::s_sendOrderBook(targetID, localBids);
    FIXAcceptor::s_sendOrderBook(targetID, localAsks);

    m_spinlock.clear();
}

void StockMarket::bufNewGlobalOrder(Order&& newOrder)
{
    try {
        std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
        m_newGlobalOrders.push(std::move(newOrder));
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void StockMarket::bufNewLocalOrder(Order&& newOrder)
{
    try {
        std::lock_guard<std::mutex> nlGuard(m_mtxNewLocalOrders);
        m_newLocalOrders.push(std::move(newOrder));
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

auto StockMarket::getNextOrder(Order& orderRef) -> bool
{
    bool good = false;
    long now = TimeSetting::getInstance().pastMilli(true);

    std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
    if (!m_newGlobalOrders.empty()) {
        orderRef = m_newGlobalOrders.front();
        if (orderRef.getMilli() < now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> nlGuard(m_mtxNewLocalOrders);
    if (!m_newLocalOrders.empty()) {
        Order* nextLocalOrder = &m_newLocalOrders.front();
        if (good) {
            if (orderRef.getMilli() >= nextLocalOrder->getMilli()) {
                orderRef = *nextLocalOrder;
                m_newLocalOrders.pop();
            } else {
                m_newGlobalOrders.pop();
            }
        } else if (nextLocalOrder->getMilli() < now) {
            good = true;
            orderRef = *nextLocalOrder;
            m_newLocalOrders.pop();
        }
    } else if (good) {
        m_newGlobalOrders.pop();
    }

    return good;
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void StockMarket::executeGlobalOrder(Order& orderRef, int size, double price, char decision)
{
    int executionSize = std::min(m_thisGlobalOrder->getSize(), orderRef.getSize());

    if (size >= 0) {
        m_thisGlobalOrder->setSize(size);
        orderRef.setSize(0);
    } else {
        m_thisGlobalOrder->setSize(0);
        orderRef.setSize(-size);
    }

    addExecutionReport({ orderRef.getSymbol(),
        price,
        executionSize,
        m_thisGlobalOrder->getTraderID(),
        orderRef.getTraderID(),
        m_thisGlobalOrder->getType(),
        orderRef.getType(),
        m_thisGlobalOrder->getOrderID(),
        orderRef.getOrderID(),
        decision,
        m_thisGlobalOrder->getDestination(),
        m_thisGlobalOrder->getTime(),
        orderRef.getTime() });
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void StockMarket::executeLocalOrder(Order& orderRef, int size, double price, char decision)
{
    int executionSize = std::min(m_thisLocalOrder->getSize(), orderRef.getSize());
    m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() - executionSize);

    if (size >= 0) {
        m_thisLocalOrder->setSize(size);
        orderRef.setSize(0);
    } else {
        m_thisLocalOrder->setSize(0);
        orderRef.setSize(-size);
    }

    addExecutionReport({ orderRef.getSymbol(),
        price,
        executionSize,
        m_thisLocalOrder->getTraderID(),
        orderRef.getTraderID(),
        m_thisLocalOrder->getType(),
        orderRef.getType(),
        m_thisLocalOrder->getOrderID(),
        orderRef.getOrderID(),
        decision,
        m_thisLocalOrder->getDestination(),
        m_thisLocalOrder->getTime(),
        orderRef.getTime() });
}

/* static */ std::atomic<bool> StockMarketList::s_isTimeout { false };

/* static */ auto StockMarketList::getInstance() -> StockMarketList::stock_market_list_t&
{
    static stock_market_list_t data;
    return data;
}

} // markets
