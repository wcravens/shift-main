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
void Stock::executeGlobalOrder(int size, Order& newOrder, char decision, const std::string& destination)
{
    int executeSize = std::min(m_thisGlobalOrder->getSize(), newOrder.getSize());

    if (size >= 0) {
        m_thisGlobalOrder->setSize(size);
        newOrder.setSize(0);
    } else {
        m_thisGlobalOrder->setSize(0);
        newOrder.setSize(-size);
    }

    executionReports.push_back({ newOrder.getSymbol(),
        m_thisGlobalOrder->getPrice(),
        executeSize,
        m_thisGlobalOrder->getTraderID(),
        newOrder.getTraderID(),
        m_thisGlobalOrder->getType(),
        newOrder.getType(),
        m_thisGlobalOrder->getOrderID(),
        newOrder.getOrderID(),
        decision,
        destination,
        m_thisGlobalOrder->getTime(),
        newOrder.getTime() });
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void Stock::executeLocalOrder(int size, Order& newOrder, char decision, const std::string& destination)
{
    int executeSize = std::min(m_thisLocalOrder->getSize(), newOrder.getSize());
    m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() - executeSize);

    if (size >= 0) {
        m_thisLocalOrder->setSize(size);
        newOrder.setSize(0);

    } else {
        m_thisLocalOrder->setSize(0);
        newOrder.setSize(-size);
    }

    executionReports.push_back({ newOrder.getSymbol(),
        m_thisLocalOrder->getPrice(),
        executeSize,
        m_thisLocalOrder->getTraderID(),
        newOrder.getTraderID(),
        m_thisLocalOrder->getType(),
        newOrder.getType(),
        m_thisLocalOrder->getOrderID(),
        newOrder.getOrderID(),
        decision,
        destination,
        m_thisLocalOrder->getTime(),
        newOrder.getTime() });
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

void Stock::doLimitBuy(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localAsks.begin();
    while (newOrder.getSize() != 0) {
        if (m_thisPriceLevel == m_localAsks.end())
            break;
        if (m_thisPriceLevel->getPrice() <= newOrder.getPrice()) {
            m_thisLocalOrder = m_thisPriceLevel->begin();
            while (newOrder.getSize() != 0) {
                if (m_thisLocalOrder == m_thisPriceLevel->end())
                    break;
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    ++m_thisLocalOrder;
                    continue;
                }
                int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
                executeLocalOrder(size, newOrder, '2', destination);
                if (size <= 0) {
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                }
            }
        }

        orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doLimitSell(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localBids.begin();
    while (newOrder.getSize() != 0) {
        if (m_thisPriceLevel == m_localBids.end())
            break;
        if (m_thisPriceLevel->getPrice() >= newOrder.getPrice()) {
            m_thisLocalOrder = m_thisPriceLevel->begin();
            while (newOrder.getSize() != 0) {
                if (m_thisLocalOrder == m_thisPriceLevel->end())
                    break;
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    ++m_thisLocalOrder;
                    continue;
                }
                int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
                executeLocalOrder(size, newOrder, '2', destination);
                if (size <= 0) {
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                }
            }
        }

        orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doMarketBuy(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localAsks.begin();
    while (newOrder.getSize() != 0) {
        if (m_thisPriceLevel == m_localAsks.end())
            break;
        m_thisLocalOrder = m_thisPriceLevel->begin();
        while (newOrder.getSize() != 0) {
            if (m_thisLocalOrder == m_thisPriceLevel->end())
                break;
            if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                ++m_thisLocalOrder;
                continue;
            }
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }
        }

        orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doMarketSell(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localBids.begin();
    while (newOrder.getSize() != 0) {
        if (m_thisPriceLevel == m_localBids.end())
            break;
        m_thisLocalOrder = m_thisPriceLevel->begin();
        while (newOrder.getSize() != 0) {
            if (m_thisLocalOrder == m_thisPriceLevel->end())
                break;
            if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                ++m_thisLocalOrder;
                continue;
            }
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }
        }

        orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doCancelBid(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localBids.begin();
    while (m_thisPriceLevel != m_localBids.end()) {
        if (m_thisPriceLevel->getPrice() > newOrder.getPrice()) {
            ++m_thisPriceLevel;
        } else
            break;
    }
    if (m_thisPriceLevel == m_localBids.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPriceLevel->getPrice() == newOrder.getPrice()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
        int undone = 1;
        while (m_thisLocalOrder != m_thisPriceLevel->end()) {
            if (m_thisLocalOrder->getOrderID() == newOrder.getOrderID()) {
                int size = m_thisLocalOrder->getSize() - newOrder.getSize();
                executeLocalOrder(size, newOrder, '4', destination);

                orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

                if (size <= 0)
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                if (m_thisPriceLevel->empty()) {
                    m_localBids.erase(m_thisPriceLevel);
                    undone = 0;
                } else if (m_thisLocalOrder != m_thisPriceLevel->begin())
                    --m_thisLocalOrder;
                break;
            } else {
                m_thisLocalOrder++;
            }
        }
        if (undone) {
            if (m_thisLocalOrder == m_thisPriceLevel->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
}

void Stock::doCancelAsk(Order& newOrder, const std::string& destination)
{
    m_thisPriceLevel = m_localAsks.begin();
    while (m_thisPriceLevel != m_localAsks.end()) {
        if (m_thisPriceLevel->getPrice() < newOrder.getPrice()) {
            ++m_thisPriceLevel;
        } else
            break;
    }
    if (m_thisPriceLevel == m_localAsks.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPriceLevel->getPrice() == newOrder.getPrice()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
        int undone = 1;
        while (m_thisLocalOrder != m_thisPriceLevel->end()) {
            if (m_thisLocalOrder->getOrderID() == newOrder.getOrderID()) {
                int size = m_thisLocalOrder->getSize() - newOrder.getSize();
                executeLocalOrder(size, newOrder, '4', destination);

                orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

                if (size <= 0)
                    m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
                if (m_thisPriceLevel->empty()) {
                    m_localAsks.erase(m_thisPriceLevel);
                    undone = 0;
                } else if (m_thisLocalOrder != m_thisPriceLevel->begin())
                    --m_thisLocalOrder;
                break;
            } else {
                m_thisLocalOrder++;
            }
        }
        if (undone) {
            if (m_thisLocalOrder == m_thisPriceLevel->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
}

void Stock::updateGlobalBids(const Order& order)
{
    orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, order.getPrice(), order.getSize(),
        order.getDestination(), globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisGlobalOrder = m_globalBids.begin();
    while (!m_globalBids.empty()) {
        if (m_thisGlobalOrder == m_globalBids.end()) {
            m_globalBids.push_back(order);
            break;
        } else if (m_thisGlobalOrder->getPrice() > order.getPrice()) {
            std::list<Order>::iterator it;
            it = m_thisGlobalOrder;
            ++m_thisGlobalOrder;
            m_globalBids.erase(it);
        } else if (m_thisGlobalOrder->getPrice() == order.getPrice()) {
            if (m_thisGlobalOrder->getDestination() == order.getDestination()) {
                m_thisGlobalOrder->setSize(order.getSize());
                break;
            } else
                m_thisGlobalOrder++;
        } else if (m_thisGlobalOrder->getPrice() < order.getPrice()) {
            m_globalBids.insert(m_thisGlobalOrder, order);
            break;
        }
    }
    if (m_globalBids.empty())
        m_globalBids.push_back(order);
}

void Stock::updateGlobalAsks(const Order& order)
{
    orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, order.getPrice(), order.getSize(),
        order.getDestination(), globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisGlobalOrder = m_globalAsks.begin();
    while (!m_globalAsks.empty()) {
        if (m_thisGlobalOrder == m_globalAsks.end()) {
            m_globalAsks.push_back(order);
            break;
        } else if (m_thisGlobalOrder->getPrice() < order.getPrice()) {
            std::list<Order>::iterator it;
            it = m_thisGlobalOrder;
            ++m_thisGlobalOrder;
            m_globalAsks.erase(it);
        } else if (m_thisGlobalOrder->getPrice() == order.getPrice()) {
            if (m_thisGlobalOrder->getDestination() == order.getDestination()) {
                m_thisGlobalOrder->setSize(order.getSize());
                break;
            } else
                m_thisGlobalOrder++;
        } else if (m_thisGlobalOrder->getPrice() > order.getPrice()) {
            m_globalAsks.insert(m_thisGlobalOrder, order);
            break;
        }
    }
    if (m_globalAsks.empty())
        m_globalAsks.push_back(order);
}

void Stock::doLocalLimitBuy(Order& newOrder, const std::string& destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0;

    // init
    m_thisPriceLevel = m_localAsks.begin();
    if (m_thisPriceLevel != m_localAsks.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }
    while (newOrder.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsks.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    // skip the quote from the same trader
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
            cout << "Global ask book is empty in dolimitbuy." << endl;
        }

        // stop if no order avaiable
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        // stop if newOrder price < best price
        double bestPrice = std::min(localBestAsk, globalBestAsk);
        if (newOrder.getPrice() < bestPrice) {
            break;
        }

        // convert to market buy order
        newOrder.setType(Order::Type::MARKET_BUY);

        // compare the price
        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // update the new price for broadcasting
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsks.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobalOrder->getSize() - newOrder.getSize();
            executeGlobalOrder(size, newOrder, '2', m_thisGlobalOrder->getDestination());
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }

        // recover to limit buy order
        newOrder.setType(Order::Type::LIMIT_BUY);
    }
}

void Stock::doLocalLimitSell(Order& newOrder, const std::string& destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0;

    // init
    m_thisPriceLevel = m_localBids.begin();
    if (m_thisPriceLevel != m_localBids.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }
    while (newOrder.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBids.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    // skip the quote from the same trader
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
            cout << "Global bid book is empty in dolimitsell." << endl;
        }

        // stop if newOrder price < best price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        // stop if newOrder price > best price
        double bestPrice = std::max(localBestBid, globalBestBid);
        if (newOrder.getPrice() > bestPrice) {
            break;
        }

        // convert to market sell order
        newOrder.setType(Order::Type::MARKET_SELL);

        // compare the price
        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // update the new price for broadcasting
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBids.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobalOrder->getSize() - newOrder.getSize();
            executeGlobalOrder(size, newOrder, '2', m_thisGlobalOrder->getDestination());
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }

        // recover to limit sell order
        newOrder.setType(Order::Type::LIMIT_SELL);
    }
}

void Stock::doLocalMarketBuy(Order& newOrder, const std::string& destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0;

    // init
    m_thisPriceLevel = m_localAsks.begin();
    if (m_thisPriceLevel != m_localAsks.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }
    while (newOrder.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsks.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    // skip the quote from the same trader
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
            cout << "Global ask book is empty in domarketbuy." << endl;
        }

        // compare the price
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes.
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // update the new price for broadcasting
            orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsks.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsks.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobalOrder->getSize() - newOrder.getSize();
            executeGlobalOrder(size, newOrder, '2', m_thisGlobalOrder->getDestination());
            orderBookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsks.pop_front();
            }
        }
    }
}

void Stock::doLocalMarketSell(Order& newOrder, const std::string& destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0;

    // init
    m_thisPriceLevel = m_localBids.begin();
    if (m_thisPriceLevel != m_localBids.end()) {
        m_thisLocalOrder = m_thisPriceLevel->begin();
    }
    while (newOrder.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBids.end()) {
            if (m_thisLocalOrder == m_thisPriceLevel->end()) {
                // if m_thisLocalOrder reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisLocalOrder = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisLocalOrder->getTraderID() == newOrder.getTraderID()) {
                    // skip the quote from the same trader
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
            cout << "Global bid book is empty in domarketsell." << endl;
        }

        // compare the price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size = m_thisLocalOrder->getSize() - newOrder.getSize(); // compare the two sizes
            executeLocalOrder(size, newOrder, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // update the new price for broadcasting
            orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBids.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBids.end()) {
                    m_thisLocalOrder = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobalOrder->getSize() - newOrder.getSize();
            executeGlobalOrder(size, newOrder, '2', m_thisGlobalOrder->getDestination());
            orderBookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobalOrder->getPrice(), m_thisGlobalOrder->getSize(),
                m_thisGlobalOrder->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBids.pop_front();
            }
        }
    }
}

void Stock::insertLocalBid(const Order& order)
{
    orderBookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, order.getPrice(), order.getSize(),
        globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisPriceLevel = m_localBids.begin();

    if (!m_localBids.empty()) {
        while (m_thisPriceLevel != m_localBids.end() && m_thisPriceLevel->getPrice() >= order.getPrice()) {
            if (m_thisPriceLevel->getPrice() == order.getPrice()) {
                m_thisPriceLevel->push_back(order);
                m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + order.getSize());

                break;
            }
            m_thisPriceLevel++;
        }

        if (m_thisPriceLevel == m_localBids.end()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());

            newPriceLevel.push_front(order);
            m_localBids.push_back(newPriceLevel);
        } else if (m_thisPriceLevel->getPrice() < order.getPrice()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());

            newPriceLevel.push_front(order);
            m_localBids.insert(m_thisPriceLevel, newPriceLevel);
        }

    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(order.getPrice());
        newPriceLevel.setSize(order.getSize());

        newPriceLevel.push_front(order);
        m_localBids.push_back(newPriceLevel);
    }
}

void Stock::insertLocalAsk(const Order& order)
{
    orderBookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, order.getPrice(), order.getSize(),
        globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisPriceLevel = m_localAsks.begin();

    if (!m_localAsks.empty()) {
        while (m_thisPriceLevel != m_localAsks.end() && m_thisPriceLevel->getPrice() <= order.getPrice()) {
            if (m_thisPriceLevel->getPrice() == order.getPrice()) {
                m_thisPriceLevel->push_back(order);
                m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + order.getSize());
                break;
            }
            m_thisPriceLevel++;
        }

        if (m_thisPriceLevel == m_localAsks.end()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());

            newPriceLevel.push_front(order);
            m_localAsks.push_back(newPriceLevel);
        } else if (m_thisPriceLevel->getPrice() > order.getPrice()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(order.getPrice());
            newPriceLevel.setSize(order.getSize());

            newPriceLevel.push_front(order);
            m_localAsks.insert(m_thisPriceLevel, newPriceLevel);
        }
    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(order.getPrice());
        newPriceLevel.setSize(order.getSize());

        newPriceLevel.push_front(order);
        m_localAsks.push_back(newPriceLevel);
    }
}
