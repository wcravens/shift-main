#include "StockMarket.h"

#include "FIXAcceptor.h"
#include "TimeSetting.h"

#include <chrono>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

StockMarket::StockMarket(const std::string& symbol)
    : m_symbol(symbol)
{
}

StockMarket::StockMarket(const StockMarket& StockMarket)
    : m_symbol(StockMarket.m_symbol)
{
}

/**
 * @brief Function to start one StockMarket matching engine, for exchange thread.
 */
void StockMarket::operator()()
{
    Order nextOrder;
    Order prevGlobalOrder;

    while (!StockMarketList::s_isTimeout) { // process orders

        if (!getNextOrder(nextOrder)) {
            std::this_thread::sleep_for(10ms);
            continue;

        } else {
            // spinlock implementation:
            // it is better than a standard lock in this scenario, since,
            // most of the time, only this thread needs access to order book data
            while (m_spinlock.test_and_set())
                continue;

            switch (nextOrder.getType()) {

            case Order::Type::LIMIT_BUY: {
                doLocalLimitBuy(nextOrder);
                if (nextOrder.getSize() != 0) {
                    insertLocalBid(nextOrder);
                    cout << "Insert Bid" << endl;
                }
                break;
            }

            case Order::Type::LIMIT_SELL: {
                doLocalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    insertLocalAsk(nextOrder);
                    cout << "Insert Ask" << endl;
                }
                break;
            }

            case Order::Type::MARKET_BUY: {
                doLocalMarketBuy(nextOrder);
                if (nextOrder.getSize() != 0) {
                    insertLocalBid(nextOrder);
                    cout << "Insert Bid" << endl;
                }
                break;
            }

            case Order::Type::MARKET_SELL: {
                doLocalMarketSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    insertLocalAsk(nextOrder);
                    cout << "Insert Ask" << endl;
                }
                break;
            }

            case Order::Type::CANCEL_BID: {
                doLocalCancelBid(nextOrder);
                std::cout << "Cancel Bid Done" << endl;
                break;
            }

            case Order::Type::CANCEL_ASK: {
                doLocalCancelAsk(nextOrder);
                std::cout << "Cancel Ask Done" << endl;
                break;
            }

            case Order::Type::TRTH_TRADE: {
                doGlobalLimitBuy(nextOrder);
                doGlobalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    auto now = TimeSetting::getInstance().simulationTimestamp();
                    executionReports.push_back({ nextOrder.getSymbol(),
                        nextOrder.getPrice(),
                        nextOrder.getSize(),
                        "T1",
                        "T2",
                        Order::Type::LIMIT_BUY,
                        Order::Type::LIMIT_SELL,
                        "O1",
                        "O2",
                        '5', // decision '5' means this is a trade update from TRTH
                        "TRTH",
                        now,
                        now });
                }
                break;
            }

            case Order::Type::TRTH_BID: {
                // if same price, size, and destination, skip
                if (nextOrder == prevGlobalOrder) {
                    break;
                }
                prevGlobalOrder = nextOrder;
                doGlobalLimitBuy(nextOrder);
                if (nextOrder.getSize() != 0)
                    updateGlobalBids(nextOrder);
                break;
            }

            case Order::Type::TRTH_ASK: {
                // if same price, size, and destination, skip
                if (nextOrder == prevGlobalOrder) {
                    break;
                }
                prevGlobalOrder = nextOrder;
                doGlobalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0)
                    updateGlobalAsks(nextOrder);
                break;
            }
            }

            // for debugging:
            // displayGlobalOrderBooks();
            // displayLocalOrderBooks();

            FIXAcceptor::getInstance()->sendExecutionReports(executionReports);
            executionReports.clear();

            FIXAcceptor::getInstance()->sendOrderBookUpdates(orderBookUpdates);
            orderBookUpdates.clear();

            m_spinlock.clear();
        }
    }
}

const std::string& StockMarket::getSymbol() const
{
    return m_symbol;
}

void StockMarket::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
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
    while (m_spinlock.test_and_set())
        continue;

    // requirements:
    // - the first order book entry must have a price <= 0.0. This will signal the BC it
    // needs to clear its current version of the order book before accepting new entries
    // - order book entries are sent from worst to best price (in reverse order) so
    // that the BC can use the same update procedure as normal order book updates

    auto now = TimeSetting::getInstance().simulationTimestamp();

    globalBids.push_back({ OrderBookEntry::Type::GLB_BID, m_symbol, 0.0, 0, now });
    for (auto rit = m_globalBids.rbegin(); rit != m_globalBids.rend(); ++rit) {
        globalBids.push_back({ OrderBookEntry::Type::GLB_BID, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now });
    }

    globalAsks.push_back({ OrderBookEntry::Type::GLB_ASK, m_symbol, 0.0, 0, now });
    for (auto rit = m_globalAsks.rbegin(); rit != m_globalAsks.rend(); ++rit) {
        globalAsks.push_back({ OrderBookEntry::Type::GLB_ASK, m_symbol, rit->getPrice(), rit->getSize(), rit->getDestination(), now });
    }

    localBids.push_back({ OrderBookEntry::Type::LOC_BID, m_symbol, 0.0, 0, now });
    for (auto rit = m_localBids.rbegin(); rit != m_localBids.rend(); ++rit) {
        localBids.push_back({ OrderBookEntry::Type::LOC_BID, m_symbol, rit->getPrice(), rit->getSize(), now });
    }

    localAsks.push_back({ OrderBookEntry::Type::LOC_ASK, m_symbol, 0.0, 0, now });
    for (auto rit = m_localAsks.rbegin(); rit != m_localAsks.rend(); ++rit) {
        localAsks.push_back({ OrderBookEntry::Type::LOC_ASK, m_symbol, rit->getPrice(), rit->getSize(), now });
    }

    FIXAcceptor::getInstance()->sendOrderBook(targetID, globalBids);
    FIXAcceptor::getInstance()->sendOrderBook(targetID, globalAsks);
    FIXAcceptor::getInstance()->sendOrderBook(targetID, localBids);
    FIXAcceptor::getInstance()->sendOrderBook(targetID, localAsks);

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

bool StockMarket::getNextOrder(Order& orderRef)
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
            } else
                m_newGlobalOrders.pop();
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

    executionReports.push_back({ orderRef.getSymbol(),
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

    executionReports.push_back({ orderRef.getSymbol(),
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

void StockMarket::orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& simulationTime)
{
    orderBookUpdates.push_back({ type, symbol, price, size, destination, simulationTime });
}

void StockMarket::orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& simulationTime)
{
    orderBookUpdates.push_back({ type, symbol, price, size, simulationTime });
}

void StockMarket::displayGlobalOrderBooks()
{
    long simulationMilli = TimeSetting::getInstance().pastMilli(true);

    cout << endl
         << "(" << simulationMilli << ") "
         << "Global Ask:"
         << endl;

    for (auto it = m_globalAsks.begin(); it != m_globalAsks.end(); ++it) {
        cout << it->getPrice() << '\t' << it->getSize() << '\t' << it->getDestination() << endl;
    }

    cout << endl
         << "(" << simulationMilli << ") "
         << "Global Bid:"
         << endl;

    for (auto it = m_globalBids.begin(); it != m_globalBids.end(); ++it) {
        cout << it->getPrice() << '\t' << it->getSize() << '\t' << it->getDestination() << endl;
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

    for (auto it = m_localAsks.begin(); it != m_localAsks.end(); ++it) {
        cout << it->getPrice() << '\t' << it->getSize() << endl;
    }

    cout << endl
         << "(" << simulationMilli << ") "
         << "Local Bid:"
         << endl;

    for (auto it = m_localBids.begin(); it != m_localBids.end(); ++it) {
        cout << it->getPrice() << '\t' << it->getSize() << endl;
    }

    cout << endl;
}

void StockMarket::doGlobalLimitBuy(Order& orderRef)
{
    m_thisPriceLevel = m_localAsks.begin();

    // see StockMarket::insertLocalAsk for more info
    double price = 0.0;
    bool update = false;

    while ((m_thisPriceLevel != m_localAsks.end()) && (orderRef.getSize() != 0)) {

        if (orderRef.getPrice() < m_thisPriceLevel->getPrice()) {
            break;
        }

        m_thisLocalOrder = m_thisPriceLevel->begin();

        if (m_thisLocalOrder->getType() == Order::Type::MARKET_SELL) {
            // if the next order in line is an unfulfilled market order
            // we should use the next price level as the order price
            // (if available and better than the matching limit order price),
            // otherwise we should use the matching limit order price
            auto nextPriceLevel = m_thisPriceLevel;
            ++nextPriceLevel;
            if ((nextPriceLevel != m_localAsks.end()) && (nextPriceLevel->getPrice() < orderRef.getPrice())) {
                price = nextPriceLevel->getPrice();
            } else {
                price = orderRef.getPrice();
            }
            update = false;
        } else {
            price = m_thisLocalOrder->getPrice();
            update = true;
        }

        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getSize() != 0)) {

            // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, price, '2');

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }
        }

        if (update) {
            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                TimeSetting::getInstance().simulationTimestamp());
        }

        // remove empty price level from local order book
        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
        }
    }
}

void StockMarket::doGlobalLimitSell(Order& orderRef)
{
    m_thisPriceLevel = m_localBids.begin();

    // see StockMarket::insertLocalBid for more info
    double price = 0.0;
    bool update = false;

    while ((m_thisPriceLevel != m_localBids.end()) && (orderRef.getSize() != 0)) {

        if (orderRef.getPrice() > m_thisPriceLevel->getPrice()) {
            break;
        }

        m_thisLocalOrder = m_thisPriceLevel->begin();

        if (m_thisLocalOrder->getType() == Order::Type::MARKET_BUY) {
            // if the next order in line is an unfulfilled market order
            // we should use the next price level as the order price
            // (if available and better than the matching limit order price),
            // otherwise we should use the matching limit order price
            auto nextPriceLevel = m_thisPriceLevel;
            ++nextPriceLevel;
            if ((nextPriceLevel != m_localBids.end()) && (nextPriceLevel->getPrice() > orderRef.getPrice())) {
                price = nextPriceLevel->getPrice();
            } else {
                price = orderRef.getPrice();
            }
            update = false;
        } else {
            price = m_thisLocalOrder->getPrice();
            update = true;
        }

        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getSize() != 0)) {

            // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, price, '2');

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }
        }

        if (update) {
            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                TimeSetting::getInstance().simulationTimestamp());
        }

        // remove empty price level from local order book
        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
        }
    }
}

void StockMarket::updateGlobalBids(Order newBestBid)
{
    // broadcast global order book update
    orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, newBestBid.getPrice(), newBestBid.getSize(),
        newBestBid.getDestination(), TimeSetting::getInstance().simulationTimestamp());

    m_thisGlobalOrder = m_globalBids.begin();

    while (m_thisGlobalOrder != m_globalBids.end()) {

        if (newBestBid.getPrice() < m_thisGlobalOrder->getPrice()) {
            m_thisGlobalOrder = m_globalBids.erase(m_thisGlobalOrder);

        } else if (newBestBid.getPrice() == m_thisGlobalOrder->getPrice()) {
            if (newBestBid.getDestination() == m_thisGlobalOrder->getDestination()) {
                m_thisGlobalOrder->setSize(newBestBid.getSize());
                return;
            } else {
                ++m_thisGlobalOrder;
            }

        } else if (newBestBid.getPrice() > m_thisGlobalOrder->getPrice()) {
            m_globalBids.insert(m_thisGlobalOrder, std::move(newBestBid));
            return;
        }
    }

    m_globalBids.push_back(std::move(newBestBid));
}

void StockMarket::updateGlobalAsks(Order newBestAsk)
{
    // broadcast global order book update
    orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, newBestAsk.getPrice(), newBestAsk.getSize(),
        newBestAsk.getDestination(), TimeSetting::getInstance().simulationTimestamp());

    m_thisGlobalOrder = m_globalAsks.begin();

    while (m_thisGlobalOrder != m_globalAsks.end()) {
        if (newBestAsk.getPrice() > m_thisGlobalOrder->getPrice()) {
            m_thisGlobalOrder = m_globalAsks.erase(m_thisGlobalOrder);

        } else if (newBestAsk.getPrice() == m_thisGlobalOrder->getPrice()) {
            if (newBestAsk.getDestination() == m_thisGlobalOrder->getDestination()) {
                m_thisGlobalOrder->setSize(newBestAsk.getSize());
                return;
            } else {
                ++m_thisGlobalOrder;
            }

        } else if (newBestAsk.getPrice() < m_thisGlobalOrder->getPrice()) {
            m_globalAsks.insert(m_thisGlobalOrder, std::move(newBestAsk));
            return;
        }
    }

    m_globalAsks.push_back(std::move(newBestAsk));
}

void StockMarket::doLocalLimitBuy(Order& orderRef)
{
    double maxAskPrice = std::numeric_limits<double>::max();
    double localBestAsk = 0.0;
    double globalBestAsk = 0.0;
    bool update = false; // see StockMarket::insertLocalAsk for more info

    // should execute as a market order
    orderRef.setType(Order::Type::MARKET_BUY);

    // init
    m_thisPriceLevel = m_localAsks.begin();
    if (m_thisPriceLevel != m_localAsks.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }

    while (orderRef.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsks.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reaches end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == orderRef.getTraderID()) {
                    // skip order from same trader
                    ++m_thisLocalOrder;
                    continue;
                } else if (m_thisLocalOrder->getType() == Order::Type::MARKET_SELL) {
                    // if the next order in line is an unfulfilled market order
                    // we should use the next price level as the order price
                    // (if available and better than the matching limit order price),
                    // otherwise we should use the matching limit order price
                    auto nextPriceLevel = m_thisPriceLevel;
                    ++nextPriceLevel;
                    if ((nextPriceLevel != m_localAsks.end()) && (nextPriceLevel->getPrice() < orderRef.getPrice())) {
                        localBestAsk = nextPriceLevel->getPrice();
                    } else {
                        localBestAsk = orderRef.getPrice();
                    }
                    update = false;
                } else {
                    localBestAsk = m_thisLocalOrder->getPrice();
                    update = true;
                }
            }
        }

        // get the best global price
        m_thisGlobalOrder = m_globalAsks.begin();
        if (m_thisGlobalOrder != m_globalAsks.end()) {
            globalBestAsk = m_thisGlobalOrder->getPrice();
        } else {
            cout << "Global ask order book is empty in doLocalLimitBuy()." << endl;
        }

        // stop if both order books are empty
        if ((localBestAsk == maxAskPrice) && (globalBestAsk == maxAskPrice)) {
            break;
        }

        // stop if new order price < best available ask price
        if (orderRef.getPrice() < std::min(localBestAsk, globalBestAsk)) {
            break;
        }

        if (localBestAsk <= globalBestAsk) { // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, localBestAsk, '2');

            if (update) {
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsks.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }

        } else { // execute in global order book
            int size = m_thisGlobalOrder->getSize() - orderRef.getSize();
            executeGlobalOrder(orderRef, size, globalBestAsk, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), TimeSetting::getInstance().simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }
    }

    // set type back to limit order
    orderRef.setType(Order::Type::LIMIT_BUY);
}

void StockMarket::doLocalLimitSell(Order& orderRef)
{
    double minBidPrice = std::numeric_limits<double>::min();
    double localBestBid = 0.0;
    double globalBestBid = 0.0;
    bool update = false; // see StockMarket::insertLocalBid for more info

    // should execute as a market order
    orderRef.setType(Order::Type::MARKET_SELL);

    // init
    m_thisPriceLevel = m_localBids.begin();
    if (m_thisPriceLevel != m_localBids.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }

    while (orderRef.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBids.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reaches end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == orderRef.getTraderID()) {
                    // skip order from same trader
                    ++m_thisLocalOrder;
                    continue;
                } else if (m_thisLocalOrder->getType() == Order::Type::MARKET_BUY) {
                    // if the next order in line is an unfulfilled market order
                    // we should use the next price level as the order price
                    // (if available and better than the matching limit order price),
                    // otherwise we should use the matching limit order price
                    auto nextPriceLevel = m_thisPriceLevel;
                    ++nextPriceLevel;
                    if ((nextPriceLevel != m_localBids.end()) && (nextPriceLevel->getPrice() > orderRef.getPrice())) {
                        localBestBid = nextPriceLevel->getPrice();
                    } else {
                        localBestBid = orderRef.getPrice();
                    }
                    update = false;
                } else {
                    localBestBid = m_thisLocalOrder->getPrice();
                    update = true;
                }
            }
        }

        // get the best global price
        m_thisGlobalOrder = m_globalBids.begin();
        if (m_thisGlobalOrder != m_globalBids.end()) {
            globalBestBid = m_thisGlobalOrder->getPrice();
        } else {
            cout << "Global bid order book is empty in doLocalLimitSell()." << endl;
        }

        // stop if both order books are empty
        if ((localBestBid == minBidPrice) && (globalBestBid == minBidPrice)) {
            break;
        }

        // stop if new order price > best available bid price
        if (orderRef.getPrice() > std::max(localBestBid, globalBestBid)) {
            break;
        }

        if (localBestBid >= globalBestBid) { // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, localBestBid, '2');

            if (update) {
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBids.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }

        } else { // execute in global order book
            int size = m_thisGlobalOrder->getSize() - orderRef.getSize();
            executeGlobalOrder(orderRef, size, globalBestBid, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), TimeSetting::getInstance().simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }
    }

    // set type back to limit order
    orderRef.setType(Order::Type::LIMIT_SELL);
}

void StockMarket::doLocalMarketBuy(Order& orderRef)
{
    double maxAskPrice = std::numeric_limits<double>::max();
    double localBestAsk = 0.0;
    double globalBestAsk = 0.0;
    bool update = false; // see StockMarket::insertLocalAsk for more info

    // init
    m_thisPriceLevel = m_localAsks.begin();
    if (m_thisPriceLevel != m_localAsks.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }

    while (orderRef.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsks.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reaches end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == orderRef.getTraderID()) {
                    // skip order from same trader
                    ++m_thisLocalOrder;
                    continue;
                } else if (m_thisLocalOrder->getType() == Order::Type::MARKET_SELL) {
                    // if the next order in line is an unfulfilled market order
                    // we should use the next price level as the order price
                    // (if available)
                    auto nextPriceLevel = m_thisPriceLevel;
                    ++nextPriceLevel;
                    if (nextPriceLevel != m_localAsks.end()) {
                        localBestAsk = nextPriceLevel->getPrice();
                        update = false;
                    } else {
                        m_thisPriceLevel = m_localAsks.end();
                        continue;
                    }
                } else {
                    localBestAsk = m_thisLocalOrder->getPrice();
                    update = true;
                }
            }
        }

        // get the best global price
        m_thisGlobalOrder = m_globalAsks.begin();
        if (m_thisGlobalOrder != m_globalAsks.end()) {
            globalBestAsk = m_thisGlobalOrder->getPrice();
        } else {
            cout << "Global ask order book is empty in doLocalMarketBuy()." << endl;
        }

        // stop if both order books are empty
        if ((localBestAsk == maxAskPrice) && (globalBestAsk == maxAskPrice)) {
            break;
        }

        if (localBestAsk <= globalBestAsk) { // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, localBestAsk, '2');

            if (update) {
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsks.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }

        } else { // execute in global order book
            int size = m_thisGlobalOrder->getSize() - orderRef.getSize();
            executeGlobalOrder(orderRef, size, globalBestAsk, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), TimeSetting::getInstance().simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }
    }
}

void StockMarket::doLocalMarketSell(Order& orderRef)
{
    double minBidPrice = std::numeric_limits<double>::min();
    double localBestBid = 0.0;
    double globalBestBid = 0.0;
    bool update = false; // see StockMarket::insertLocalBid for more info

    // init
    m_thisPriceLevel = m_localBids.begin();
    if (m_thisPriceLevel != m_localBids.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }

    while (orderRef.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBids.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reaches end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == orderRef.getTraderID()) {
                    // skip order from same trader
                    ++m_thisLocalOrder;
                    continue;
                } else if (m_thisLocalOrder->getType() == Order::Type::MARKET_BUY) {
                    // if the next order in line is an unfulfilled market order
                    // we should use the next price level as the order price
                    // (if available)
                    auto nextPriceLevel = m_thisPriceLevel;
                    ++nextPriceLevel;
                    if (nextPriceLevel != m_localBids.end()) {
                        localBestBid = nextPriceLevel->getPrice();
                        update = false;
                    } else {
                        m_thisPriceLevel = m_localBids.end();
                        continue;
                    }
                } else {
                    localBestBid = m_thisLocalOrder->getPrice();
                    update = true;
                }
            }
        }

        // get the best global price
        m_thisGlobalOrder = m_globalBids.begin();
        if (m_thisGlobalOrder != m_globalBids.end()) {
            globalBestBid = m_thisGlobalOrder->getPrice();
        } else {
            cout << "Global bid order book is empty in doLocalMarketSell()." << endl;
        }

        // stop if both order books are empty
        if ((localBestBid == minBidPrice) && (globalBestBid == minBidPrice)) {
            break;
        }

        if (localBestBid >= globalBestBid) { // execute in local order book
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, localBestBid, '2');

            if (update) {
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove executed order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBids.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }

        } else { // execute in global order book
            int size = m_thisGlobalOrder->getSize() - orderRef.getSize();
            executeGlobalOrder(orderRef, size, globalBestBid, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), TimeSetting::getInstance().simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }
    }
}

void StockMarket::doLocalCancelBid(Order& orderRef)
{
    m_thisPriceLevel = m_localBids.begin();

    if (orderRef.getPrice() > 0.0) { // not a market order
        // find right price level for new order
        while ((m_thisPriceLevel != m_localBids.end()) && (orderRef.getPrice() < m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }
    }

    if (m_thisPriceLevel == m_localBids.end()) { // did not find any orders
        cout << "No such order to be canceled." << endl;

    } else if ((orderRef.getPrice() > 0.0) && (orderRef.getPrice() != m_thisPriceLevel->getPrice())) { // did not find the right price for non-market orders
        cout << "No such order to be canceled." << endl;

    } else {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        // find right order id to cancel
        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, 0.0, '4'); // cancellation orders have executed price = 0.0

            if (m_thisLocalOrder->getType() != Order::Type::MARKET_BUY) { // see StockMarket::insertLocalBid for more info
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localBids.erase(m_thisPriceLevel);
            }

        } else { // did not find any order which matched the right order id
            cout << "No such order to be canceled." << endl;
        }
    }
}

void StockMarket::doLocalCancelAsk(Order& orderRef)
{
    m_thisPriceLevel = m_localAsks.begin();

    if (orderRef.getPrice() > 0.0) { // not a market order
        // find right price level for new order
        while ((m_thisPriceLevel != m_localAsks.end()) && (orderRef.getPrice() > m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }
    }

    if (m_thisPriceLevel == m_localAsks.end()) { // did not find any orders
        cout << "No such order to be canceled." << endl;

    } else if ((orderRef.getPrice() > 0.0) && (orderRef.getPrice() != m_thisPriceLevel->getPrice())) { // did not find the right price for non-market orders
        cout << "No such order to be canceled." << endl;

    } else {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        // find right order id to cancel
        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, 0.0, '4'); // cancellation orders have executed price = 0.0

            if (m_thisLocalOrder->getType() != Order::Type::MARKET_SELL) { // see StockMarket::insertLocalAsk for more info
                // broadcast local order book update
                orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    TimeSetting::getInstance().simulationTimestamp());
            }

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localAsks.erase(m_thisPriceLevel);
            }

        } else { // did not find any order which matched the right order id
            cout << "No such order to be canceled." << endl;
        }
    }
}

void StockMarket::insertLocalBid(Order newBid)
{
    // market orders are only added to the order book if the order book in the other side is currently empty
    // they do, however, have to receive "special threatment":
    // - their insertion/exclusion in/from the order book should not be broadcasted (they are hidden)
    // - they must have a price such that they are always guaranteed first place in the order book
    // - when matching other orders, they should assume the price of the second price level (current best price)
    // - otherwise, they should assume the price of the matching limit order (and do not match with market orders)
    if (newBid.getType() == Order::Type::MARKET_BUY) {
        newBid.setPrice(std::numeric_limits<double>::max());
    }

    if (!m_localBids.empty()) {
        m_thisPriceLevel = m_localBids.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localBids.end()) && (newBid.getPrice() < m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (newBid.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newBid.getSize());
            m_thisPriceLevel->push_back(std::move(newBid));

        } else if (newBid.getPrice() > m_thisPriceLevel->getPrice()) { // new order is new best bid or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newBid.getPrice());
            newPriceLevel.setSize(newBid.getSize());
            newPriceLevel.push_back(std::move(newBid));

            m_thisPriceLevel = m_localBids.insert(m_thisPriceLevel, std::move(newPriceLevel));

        } else { // m_thisPriceLevel == m_localBids.end()
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newBid.getPrice());
            newPriceLevel.setSize(newBid.getSize());
            newPriceLevel.push_back(std::move(newBid));

            m_localBids.push_back(std::move(newPriceLevel));
            m_thisPriceLevel = m_localBids.end();
            --m_thisPriceLevel;
        }

    } else { // m_localBids.empty()
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newBid.getPrice());
        newPriceLevel.setSize(newBid.getSize());
        newPriceLevel.push_back(std::move(newBid));

        m_localBids.push_back(std::move(newPriceLevel));
        m_thisPriceLevel = m_localBids.begin();
    }

    if (newBid.getType() != Order::Type::MARKET_BUY) {
        // broadcast local order book update
        orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            TimeSetting::getInstance().simulationTimestamp());
    }
}

void StockMarket::insertLocalAsk(Order newAsk)
{
    // market orders are only added to the order book if the order book in the other side is currently empty
    // they do, however, have to receive "special threatment":
    // - their insertion/exclusion in/from the order book should not be broadcasted (they are hidden)
    // - they must have a price such that they are always guaranteed first place in the order book
    // - when matching other orders, they should assume the price of the second price level (current best price)
    // - otherwise, they should assume the price of the matching limit order (and do not match with market orders)
    if (newAsk.getType() == Order::Type::MARKET_SELL) {
        newAsk.setPrice(std::numeric_limits<double>::min());
    }

    if (!m_localAsks.empty()) {
        m_thisPriceLevel = m_localAsks.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localAsks.end()) && (newAsk.getPrice() > m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (newAsk.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newAsk.getSize());
            m_thisPriceLevel->push_back(std::move(newAsk));

        } else if (newAsk.getPrice() < m_thisPriceLevel->getPrice()) { // new order is new best ask or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newAsk.getPrice());
            newPriceLevel.setSize(newAsk.getSize());
            newPriceLevel.push_back(std::move(newAsk));

            m_thisPriceLevel = m_localAsks.insert(m_thisPriceLevel, std::move(newPriceLevel));

        } else { // m_thisPriceLevel == m_localAsks.end()
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newAsk.getPrice());
            newPriceLevel.setSize(newAsk.getSize());
            newPriceLevel.push_back(std::move(newAsk));

            m_localAsks.push_back(std::move(newPriceLevel));
            m_thisPriceLevel = m_localAsks.end();
            --m_thisPriceLevel;
        }

    } else { // m_localAsks.empty()
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newAsk.getPrice());
        newPriceLevel.setSize(newAsk.getSize());
        newPriceLevel.push_back(std::move(newAsk));

        m_localAsks.push_back(std::move(newPriceLevel));
        m_thisPriceLevel = m_localAsks.begin();
    }

    if (newAsk.getType() != Order::Type::MARKET_SELL) {
        // broadcast local order book update
        orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            TimeSetting::getInstance().simulationTimestamp());
    }
}

/* static */ std::atomic<bool> StockMarketList::s_isTimeout { false };

/* static */ StockMarketList::stock_market_list_t& StockMarketList::getInstance()
{
    static stock_market_list_t data;
    return data;
}
