#include "markets/Market.h"

#include "TimeSetting.h"

#include <shift/miscutils/terminal/Common.h>

namespace markets {

Market::Market(std::string symbol)
    : m_symbol { std::move(symbol) }
{
}

auto Market::getSymbol() const -> const std::string&
{
    return m_symbol;
}

void Market::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void Market::displayGlobalOrderBooks()
{
    auto simulationMS = TimeSetting::getInstance().pastMilli(true);

    cout << endl
         << "(" << simulationMS << ") "
         << "Global Ask:"
         << endl;

    for (const auto& globalAsk : m_globalAsks) {
        cout << globalAsk.getPrice() << '\t' << globalAsk.getSize() << '\t' << globalAsk.getDestination() << endl;
    }

    cout << endl
         << "(" << simulationMS << ") "
         << "Global Bid:"
         << endl;

    for (const auto& globalBid : m_globalBids) {
        cout << globalBid.getPrice() << '\t' << globalBid.getSize() << '\t' << globalBid.getDestination() << endl;
    }

    cout << endl;
}

void Market::displayLocalOrderBooks()
{
    auto simulationMS = TimeSetting::getInstance().pastMilli(true);

    cout << endl
         << "(" << simulationMS << ") "
         << "Local Ask:"
         << endl;

    for (const auto& localAsk : m_localAsks) {
        cout << localAsk.getPrice() << '\t' << localAsk.getSize() << endl;
    }

    cout << endl
         << "(" << simulationMS << ") "
         << "Local Bid:"
         << endl;

    for (const auto& localBid : m_localBids) {
        cout << localBid.getPrice() << '\t' << localBid.getSize() << endl;
    }

    cout << endl;
}

void Market::sendOrderBookData(const std::string& targetID /* = "" */, bool includeGlobal /* = true */, int maxLevel /* = std::numeric_limits<int>::max() */)
{
    // temporary vectors to hold order book entries
    std::vector<OrderBookEntry> globalBids;
    std::vector<OrderBookEntry> globalAsks;
    std::vector<OrderBookEntry> localBids;
    std::vector<OrderBookEntry> localAsks;

    // spinlock implementation:
    // it is better than a standard lock in this scenario, since
    // this function will only be executed a couple of times during a simulation
    std::lock_guard<shift::concurrency::Spinlock> guard(m_spinlock);

    // requirements:
    // - the first order book entry must have a price <= 0.0. This will signal the BC it
    // needs to clear its current version of the order book before accepting new entries
    // - order book entries are sent from worst to best price (in reverse order) so
    // that the BC can use the same update procedure as normal order book updates

    int size = 0;
    auto now = TimeSetting::getInstance().simulationTimestamp();

    if (includeGlobal) {
        size = m_globalBids.size();
        globalBids.emplace_back(OrderBookEntry::Type::GLB_BID, m_symbol, 0.0, 0, now);
        for (auto rit = m_globalBids.rbegin(); rit != m_globalBids.rend(); ++rit) {
            if (--size < maxLevel) {
                globalBids.emplace_back(OrderBookEntry::Type::GLB_BID, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now);
            }
        }

        size = m_globalAsks.size();
        globalAsks.emplace_back(OrderBookEntry::Type::GLB_ASK, m_symbol, 0.0, 0, now);
        for (auto rit = m_globalAsks.rbegin(); rit != m_globalAsks.rend(); ++rit) {
            if (--size < maxLevel) {
                globalAsks.emplace_back(OrderBookEntry::Type::GLB_ASK, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now);
            }
        }
    }

    size = m_localBids.size();
    localBids.emplace_back(OrderBookEntry::Type::LOC_BID, m_symbol, 0.0, 0, now);
    for (auto rit = m_localBids.rbegin(); rit != m_localBids.rend(); ++rit) {
        if (--size < maxLevel) {
            if (rit->begin()->getType() != Order::Type::MARKET_BUY) { // market orders are hidden
                localBids.emplace_back(OrderBookEntry::Type::LOC_BID, m_symbol, rit->getPrice(), rit->getSize(), now);
            }
        }
    }

    size = m_localAsks.size();
    localAsks.emplace_back(OrderBookEntry::Type::LOC_ASK, m_symbol, 0.0, 0, now);
    for (auto rit = m_localAsks.rbegin(); rit != m_localAsks.rend(); ++rit) {
        if (--size < maxLevel) {
            if (rit->begin()->getType() != Order::Type::MARKET_SELL) { // market orders are hidden
                localAsks.emplace_back(OrderBookEntry::Type::LOC_ASK, m_symbol, rit->getPrice(), rit->getSize(), now);
            }
        }
    }

    if (includeGlobal) {
        FIXAcceptor::getInstance().sendOrderBook(globalBids, targetID);
        FIXAcceptor::getInstance().sendOrderBook(globalAsks, targetID);
    }

    FIXAcceptor::getInstance().sendOrderBook(localBids, targetID);
    FIXAcceptor::getInstance().sendOrderBook(localAsks, targetID);
}

void Market::bufNewGlobalOrder(Order&& newOrder)
{
    try {
        std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
        m_newGlobalOrders.push(std::move(newOrder));
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void Market::bufNewLocalOrder(Order&& newOrder)
{
    try {
        std::lock_guard<std::mutex> nlGuard(m_mtxNewLocalOrders);
        m_newLocalOrders.push(std::move(newOrder));
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

auto Market::getNextOrder(Order& orderRef) -> bool
{
    bool good = false;
    auto simulationMS = TimeSetting::getInstance().pastMilli(true);

    std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
    if (!m_newGlobalOrders.empty()) {
        orderRef = m_newGlobalOrders.front();
        if (orderRef.getMilli() < simulationMS) {
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
        } else if (nextLocalOrder->getMilli() < simulationMS) {
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
void Market::executeGlobalOrder(Order& orderRef, int size, double price, char decision)
{
    int executionSize = std::min(m_thisGlobalOrder->getSize(), orderRef.getSize());

    if (size >= 0) {
        m_thisGlobalOrder->setSize(size);
        orderRef.setSize(0);
    } else {
        m_thisGlobalOrder->setSize(0);
        orderRef.setSize(-size);
    }

    addExecutionReport({ m_symbol,
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
void Market::executeLocalOrder(Order& orderRef, int size, double price, char decision)
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

    addExecutionReport({ m_symbol,
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

/* static */ std::atomic<bool> MarketList::s_isTimeout { false };

/* static */ auto MarketList::getInstance() -> MarketList::market_list_t&
{
    static market_list_t data;
    return data;
}

} // markets
