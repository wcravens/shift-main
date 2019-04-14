#include "Stock.h"

#include "TimeSetting.h"

#include <shift/miscutils/terminal/Common.h>

Stock::Stock(const std::string& symbol)
    : m_symbol(symbol)
{
}

Stock::Stock(const Stock& stock)
    : m_symbol(stock.m_symbol)
{
}

const std::string& Stock::getSymbol() const
{
    return m_symbol;
}

void Stock::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void Stock::bufNewGlobalOrder(const Order& order)
{
    try {
        std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
        m_newGlobalOrders.push(order);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void Stock::bufNewLocalOrder(const Order& order)
{
    try {
        std::lock_guard<std::mutex> nlGuard(m_mtxNewLocalOrders);
        m_newLocalOrders.push(order);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

bool Stock::getNextOrder(Order& nextOrder)
{
    bool good = false;
    long now = globalTimeSetting.pastMilli(true);

    std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobalOrders);
    if (!m_newGlobalOrders.empty()) {
        nextOrder = m_newGlobalOrders.front();
        if (nextOrder.getMilli() < now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> nlGuard(m_mtxNewLocalOrders);
    if (!m_newLocalOrders.empty()) {
        Order* nextLocalOrder = &m_newLocalOrders.front();
        if (good) {
            if (nextOrder.getMilli() >= nextLocalOrder->getMilli()) {
                nextOrder = *nextLocalOrder;
                m_newLocalOrders.pop();
            } else
                m_newGlobalOrders.pop();
        } else if (nextLocalOrder->getMilli() < now) {
            good = true;
            nextOrder = *nextLocalOrder;
            m_newLocalOrders.pop();
        }
    } else if (good) {
        m_newGlobalOrders.pop();
    }

    return good;
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void Stock::executeGlobalOrder(int size, Order& orderRef, char decision)
{
    int executeSize = std::min(m_thisGlobalOrder->getSize(), orderRef.getSize());

    if (size >= 0) {
        m_thisGlobalOrder->setSize(size);
        orderRef.setSize(0);
    } else {
        m_thisGlobalOrder->setSize(0);
        orderRef.setSize(-size);
    }

    executionReports.push_back({ orderRef.getSymbol(),
        m_thisGlobalOrder->getPrice(),
        executeSize,
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
void Stock::executeLocalOrder(int size, Order& orderRef, char decision)
{
    int executeSize = std::min(m_thisLocalOrder->getSize(), orderRef.getSize());
    m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() - executeSize);

    if (size >= 0) {
        m_thisLocalOrder->setSize(size);
        orderRef.setSize(0);
    } else {
        m_thisLocalOrder->setSize(0);
        orderRef.setSize(-size);
    }

    executionReports.push_back({ orderRef.getSymbol(),
        m_thisLocalOrder->getPrice(),
        executeSize,
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

void Stock::orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time)
{
    orderBookUpdates.push_back({ type, symbol, price, size, destination, time });
}

void Stock::orderBookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time)
{
    orderBookUpdates.push_back({ type, symbol, price, size, time });
}

void Stock::showGlobalOrderBooks()
{
    std::stringstream ss;
    std::string output;

    long simulationMilli = globalTimeSetting.pastMilli(true);

    ss << simulationMilli << ';'
       << "Global: "
       << ';';

    unsigned int depth = m_globalAsks.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisGlobalOrder = m_globalAsks.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobalOrder->getPrice() << ',' << m_thisGlobalOrder->getSize() << ',' << m_thisGlobalOrder->getDestination() << ',';
        m_thisGlobalOrder++;
    }
    ss << ';';

    depth = m_globalBids.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisGlobalOrder = m_globalBids.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobalOrder->getPrice() << ',' << m_thisGlobalOrder->getSize() << ',' << m_thisGlobalOrder->getDestination() << ',';
        m_thisGlobalOrder++;
    }
    ss << ';';

    output = ss.str();
}

void Stock::showLocalOrderBooks()
{
    std::stringstream ss;
    std::string output;

    long simulationMilli = globalTimeSetting.pastMilli(true);

    ss << simulationMilli << ';'
       << "Local: "
       << ';';

    unsigned int depth = m_localAsks.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisPriceLevel = m_localAsks.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPriceLevel->getPrice() << ',' << m_thisPriceLevel->getSize() << ',';
        m_thisPriceLevel++;
    }
    ss << ';';

    depth = m_localBids.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisPriceLevel = m_localBids.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPriceLevel->getPrice() << ',' << m_thisPriceLevel->getSize() << ',';
        m_thisPriceLevel++;
    }
    ss << ';';

    output = ss.str();
}

void Stock::doGlobalLimitBuy(Order& orderRef)
{
    m_thisPriceLevel = m_localAsks.begin();

    while ((m_thisPriceLevel != m_localAsks.end()) && (orderRef.getSize() != 0)) {

        if (orderRef.getPrice() >= m_thisPriceLevel->getPrice()) {
            m_thisLocalOrder = m_thisPriceLevel->begin();

            while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getSize() != 0)) {

                // execute in local order book
                int size = m_thisLocalOrder->getSize() - orderRef.getSize();
                executeLocalOrder(size, orderRef, '2');

                // remove executed order from local order book
                if (size <= 0) {
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                }
            }

        } else {
            break;
        }

        // broadcast local order book update
        orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp());

        // remove empty price level from local order book
        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
        }
    }
}

void Stock::doGlobalLimitSell(Order& orderRef)
{
    m_thisPriceLevel = m_localBids.begin();

    while ((m_thisPriceLevel != m_localBids.end()) && (orderRef.getSize() != 0)) {

        if (orderRef.getPrice() <= m_thisPriceLevel->getPrice()) {
            m_thisLocalOrder = m_thisPriceLevel->begin();

            while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getSize() != 0)) {

                // execute in local order book
                int size = m_thisLocalOrder->getSize() - orderRef.getSize();
                executeLocalOrder(size, orderRef, '2');

                // remove executed order from local order book
                if (size <= 0) {
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                }
            }
        } else {
            break;
        }

        // broadcast local order book update
        orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp());

        // remove empty price level from local order book
        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
        }
    }
}

void Stock::updateGlobalBids(const Order& newBestBid)
{
    // broadcast global order book update
    orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, newBestBid.getPrice(), newBestBid.getSize(),
        newBestBid.getDestination(), globalTimeSetting.simulationTimestamp());

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
            m_globalBids.insert(m_thisGlobalOrder, newBestBid);
            return;
        }
    }

    m_globalBids.push_back(newBestBid);
}

void Stock::updateGlobalAsks(const Order& newBestAsk)
{
    // broadcast global order book update
    orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, newBestAsk.getPrice(), newBestAsk.getSize(),
        newBestAsk.getDestination(), globalTimeSetting.simulationTimestamp());

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
            m_globalAsks.insert(m_thisGlobalOrder, newBestAsk);
            return;
        }
    }

    m_globalAsks.push_back(newBestAsk);
}

void Stock::doLocalLimitBuy(Order& orderRef)
{
    double maxAskPrice = std::numeric_limits<double>::max();
    double localBestAsk = 0.0;
    double globalBestAsk = 0.0;

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
                } else {
                    localBestAsk = m_thisLocalOrder->getPrice();
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
            executeLocalOrder(size, orderRef, '2');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

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
            executeGlobalOrder(size, orderRef, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }
    }

    // set type back to limit order
    orderRef.setType(Order::Type::LIMIT_BUY);
}

void Stock::doLocalLimitSell(Order& orderRef)
{
    double minBidPrice = std::numeric_limits<double>::min();
    double localBestBid = 0.0;
    double globalBestBid = 0.0;

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
                } else {
                    localBestBid = m_thisLocalOrder->getPrice();
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
            executeLocalOrder(size, orderRef, '2');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

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
            executeGlobalOrder(size, orderRef, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }
    }

    // set type back to limit order
    orderRef.setType(Order::Type::LIMIT_SELL);
}

void Stock::doLocalMarketBuy(Order& orderRef)
{
    double maxAskPrice = std::numeric_limits<double>::max();
    double localBestAsk = 0.0;
    double globalBestAsk = 0.0;

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
                } else {
                    localBestAsk = m_thisLocalOrder->getPrice();
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
            executeLocalOrder(size, orderRef, '2');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

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
            executeGlobalOrder(size, orderRef, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }
    }
}

void Stock::doLocalMarketSell(Order& orderRef)
{
    double minBidPrice = std::numeric_limits<double>::min();
    double localBestBid = 0.0;
    double globalBestBid = 0.0;

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
                } else {
                    localBestBid = m_thisLocalOrder->getPrice();
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
            executeLocalOrder(size, orderRef, '2');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

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
            executeGlobalOrder(size, orderRef, '2');

            // broadcast global order book update
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());

            // remove executed order from global order book
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }
    }
}

void Stock::doLocalCancelBid(Order& orderRef)
{
    // find right price level for new order
    m_thisPriceLevel = m_localBids.begin();
    while ((m_thisPriceLevel != m_localBids.end()) && (orderRef.getPrice() < m_thisPriceLevel->getPrice())) {
        ++m_thisPriceLevel;
    }

    if (m_thisPriceLevel == m_localBids.end()) {
        cout << "No such order to be canceled." << endl;
        return;
    }

    if (orderRef.getPrice() == m_thisPriceLevel->getPrice()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(size, orderRef, '4');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localBids.erase(m_thisPriceLevel);
            }

        } else {
            cout << "No such order to be canceled." << endl;
        }
    } else {
        cout << "No such order to be canceled." << endl;
    }
}

void Stock::doLocalCancelAsk(Order& orderRef)
{
    // find right price level for new order
    m_thisPriceLevel = m_localAsks.begin();
    while ((m_thisPriceLevel != m_localAsks.end()) && (orderRef.getPrice() > m_thisPriceLevel->getPrice())) {
        ++m_thisPriceLevel;
    }

    if (m_thisPriceLevel == m_localAsks.end()) {
        cout << "No such order to be canceled." << endl;
        return;
    }

    if (orderRef.getPrice() == m_thisPriceLevel->getPrice()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(size, orderRef, '4');

            // broadcast local order book update
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localAsks.erase(m_thisPriceLevel);
            }

        } else {
            cout << "No such order to be canceled." << endl;
        }
    } else {
        cout << "No such order to be canceled." << endl;
    }
}

void Stock::insertLocalBid(const Order& order)
{
    // broadcast local order book update
    orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, order.getPrice(), order.getSize(),
        globalTimeSetting.simulationTimestamp());

    if (!m_localBids.empty()) {
        m_thisPriceLevel = m_localBids.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localBids.end()) && (order.getPrice() < m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (order.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + order.getSize());
            m_thisPriceLevel->push_back(order);

        } else if (order.getPrice() > m_thisPriceLevel->getPrice()) { // new order is new best bid or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());
            newPriceLevel.push_back(order);

            m_localBids.insert(m_thisPriceLevel, newPriceLevel);

        } else { // (m_thisPriceLevel == m_localBids.end())
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());
            newPriceLevel.push_back(order);

            m_localBids.push_back(newPriceLevel);
        }

    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(order.getPrice());
        newPriceLevel.setSize(order.getSize());
        newPriceLevel.push_back(order);

        m_localBids.push_back(newPriceLevel);
    }
}

void Stock::insertLocalAsk(const Order& order)
{
    // broadcast local order book update
    orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, order.getPrice(), order.getSize(),
        globalTimeSetting.simulationTimestamp());

    if (!m_localAsks.empty()) {
        m_thisPriceLevel = m_localAsks.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localAsks.end()) && (order.getPrice() > m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (order.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + order.getSize());
            m_thisPriceLevel->push_back(order);

        } else if (order.getPrice() < m_thisPriceLevel->getPrice()) { // new order is new best ask or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());
            newPriceLevel.push_back(order);

            m_localAsks.insert(m_thisPriceLevel, newPriceLevel);

        } else { // (m_thisPriceLevel == m_localAsks.end())
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());
            newPriceLevel.push_back(order);

            m_localAsks.push_back(newPriceLevel);
        }

    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(order.getPrice());
        newPriceLevel.setSize(order.getSize());
        newPriceLevel.push_back(order);

        m_localAsks.push_back(newPriceLevel);
    }
}
