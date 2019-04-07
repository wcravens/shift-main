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

void Stock::bufNewGlobal(const Quote& quote)
{
    try {
        std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobal);
        m_newGlobal.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void Stock::bufNewLocal(const Quote& quote)
{
    try {
        std::lock_guard<std::mutex> nlGuard(m_mtxNewLocal);
        m_newLocal.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

bool Stock::getNewQuote(Quote& quote)
{
    bool good = false;
    long now = globalTimeSetting.pastMilli(true);

    std::lock_guard<std::mutex> ngGuard(m_mtxNewGlobal);
    if (!m_newGlobal.empty()) {
        quote = m_newGlobal.front();
        if (quote.getMilli() < now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> nlGuard(m_mtxNewLocal);
    if (!m_newLocal.empty()) {
        Quote* newLocal = &m_newLocal.front();
        if (good) {
            if (quote.getMilli() >= newLocal->getMilli()) {
                quote = *newLocal;
                m_newLocal.pop();
            } else
                m_newGlobal.pop();
        } else if (newLocal->getMilli() < now) {
            good = true;
            quote = *newLocal;
            m_newLocal.pop();
        }
    } else if (good) {
        m_newGlobal.pop();
    }

    return good;
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void Stock::execute(int size, Quote& newQuote, char decision, const std::string& destination)
{
    int executeSize = std::min(m_thisQuote->getSize(), newQuote.getSize());
    m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() - executeSize);

    if (size >= 0) {
        m_thisQuote->setSize(size);
        newQuote.setSize(0);

    } else {
        m_thisQuote->setSize(0);
        newQuote.setSize(-size);
    }

    auto now = globalTimeSetting.simulationTimestamp();
    ExecutionReport report{
        newQuote.getSymbol(),
        m_thisQuote->getPrice(),
        executeSize,
        m_thisQuote->getTraderID(),
        newQuote.getTraderID(),
        m_thisQuote->getOrderType(),
        newQuote.getOrderType(),
        m_thisQuote->getOrderID(),
        newQuote.getOrderID(),
        decision,
        destination,
        now,
        m_thisQuote->getTime(),
        newQuote.getTime()
    };

    executionReports.push_back(report);
}

// decision '2' means this is a trade record, '4' means cancel record; record trade or cancel with object actions
void Stock::executeGlobal(int size, Quote& newQuote, char decision, const std::string& destination)
{
    int executeSize = std::min(m_thisGlobal->getSize(), newQuote.getSize());

    if (size >= 0) {
        m_thisGlobal->setSize(size);
        newQuote.setSize(0);
    } else {
        m_thisGlobal->setSize(0);
        newQuote.setSize(-size);
    }

    auto now = globalTimeSetting.simulationTimestamp();
    ExecutionReport report{
        newQuote.getSymbol(),
        m_thisGlobal->getPrice(),
        executeSize,
        m_thisGlobal->getTraderID(),
        newQuote.getTraderID(),
        m_thisGlobal->getOrderType(),
        newQuote.getOrderType(),
        m_thisGlobal->getOrderID(),
        newQuote.getOrderID(),
        decision,
        destination,
        now,
        m_thisGlobal->getTime(),
        newQuote.getTime()
    };

    executionReports.push_back(report);
}

void Stock::doLimitBuy(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localAsk.begin();
    while (newQuote.getSize() != 0) {
        if (m_thisPriceLevel == m_localAsk.end())
            break;
        if (m_thisPriceLevel->getPrice() <= newQuote.getPrice()) {
            m_thisQuote = m_thisPriceLevel->begin();
            while (newQuote.getSize() != 0) {
                if (m_thisQuote == m_thisPriceLevel->end())
                    break;
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
                execute(size, newQuote, '2', destination);
                if (size <= 0) {
                    m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
                }
            }
        }

        bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsk.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doLimitSell(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localBid.begin();
    while (newQuote.getSize() != 0) {
        if (m_thisPriceLevel == m_localBid.end())
            break;
        if (m_thisPriceLevel->getPrice() >= newQuote.getPrice()) {
            m_thisQuote = m_thisPriceLevel->begin();
            while (newQuote.getSize() != 0) {
                if (m_thisQuote == m_thisPriceLevel->end())
                    break;
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
                execute(size, newQuote, '2', destination);
                if (size <= 0) {
                    m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
                }
            }
        }

        bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBid.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doMarketBuy(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localAsk.begin();
    while (newQuote.getSize() != 0) {
        if (m_thisPriceLevel == m_localAsk.end())
            break;
        m_thisQuote = m_thisPriceLevel->begin();
        while (newQuote.getSize() != 0) {
            if (m_thisQuote == m_thisPriceLevel->end())
                break;
            if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }
        }

        bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localAsk.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doMarketSell(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localBid.begin();
    while (newQuote.getSize() != 0) {
        if (m_thisPriceLevel == m_localBid.end())
            break;
        m_thisQuote = m_thisPriceLevel->begin();
        while (newQuote.getSize() != 0) {
            if (m_thisQuote == m_thisPriceLevel->end())
                break;
            if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }
        }

        bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

        if (m_thisPriceLevel->empty()) {
            m_thisPriceLevel = m_localBid.erase(m_thisPriceLevel);
        } else
            ++m_thisPriceLevel;
    }
}

void Stock::doCancelBid(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localBid.begin();
    while (m_thisPriceLevel != m_localBid.end()) {
        if (m_thisPriceLevel->getPrice() > newQuote.getPrice()) {
            ++m_thisPriceLevel;
        } else
            break;
    }
    if (m_thisPriceLevel == m_localBid.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPriceLevel->getPrice() == newQuote.getPrice()) {
        m_thisQuote = m_thisPriceLevel->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPriceLevel->end()) {
            if (m_thisQuote->getOrderID() == newQuote.getOrderID()) {
                int size = m_thisQuote->getSize() - newQuote.getSize();
                execute(size, newQuote, '4', destination);

                bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

                if (size <= 0)
                    m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
                if (m_thisPriceLevel->empty()) {
                    m_localBid.erase(m_thisPriceLevel);
                    undone = 0;
                } else if (m_thisQuote != m_thisPriceLevel->begin())
                    --m_thisQuote;
                break;
            } else {
                m_thisQuote++;
            }
        }
        if (undone) {
            if (m_thisQuote == m_thisPriceLevel->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
}

void Stock::doCancelAsk(Quote& newQuote, const std::string& destination)
{
    m_thisPriceLevel = m_localAsk.begin();
    while (m_thisPriceLevel != m_localAsk.end()) {
        if (m_thisPriceLevel->getPrice() < newQuote.getPrice()) {
            ++m_thisPriceLevel;
        } else
            break;
    }
    if (m_thisPriceLevel == m_localAsk.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPriceLevel->getPrice() == newQuote.getPrice()) {
        m_thisQuote = m_thisPriceLevel->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPriceLevel->end()) {
            if (m_thisQuote->getOrderID() == newQuote.getOrderID()) {
                int size = m_thisQuote->getSize() - newQuote.getSize();
                execute(size, newQuote, '4', destination);

                bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                    globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting

                if (size <= 0)
                    m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
                if (m_thisPriceLevel->empty()) {
                    m_localAsk.erase(m_thisPriceLevel);
                    undone = 0;
                } else if (m_thisQuote != m_thisPriceLevel->begin())
                    --m_thisQuote;
                break;
            } else {
                m_thisQuote++;
            }
        }
        if (undone) {
            if (m_thisQuote == m_thisPriceLevel->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
}

void Stock::bidInsert(const Quote& newQuote)
{
    bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, newQuote.getPrice(), newQuote.getSize(),
        globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisPriceLevel = m_localBid.begin();

    if (!m_localBid.empty()) {
        while (m_thisPriceLevel != m_localBid.end() && m_thisPriceLevel->getPrice() >= newQuote.getPrice()) {
            if (m_thisPriceLevel->getPrice() == newQuote.getPrice()) {
                m_thisPriceLevel->push_back(newQuote);
                m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newQuote.getSize());

                break;
            }
            m_thisPriceLevel++;
        }

        if (m_thisPriceLevel == m_localBid.end()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newQuote.getPrice());
            newPriceLevel.setSize(newQuote.getSize());

            newPriceLevel.push_front(newQuote);
            m_localBid.push_back(newPriceLevel);
        } else if (m_thisPriceLevel->getPrice() < newQuote.getPrice()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newQuote.getPrice());
            newPriceLevel.setSize(newQuote.getSize());

            newPriceLevel.push_front(newQuote);
            m_localBid.insert(m_thisPriceLevel, newPriceLevel);
        }

    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newQuote.getPrice());
        newPriceLevel.setSize(newQuote.getSize());

        newPriceLevel.push_front(newQuote);
        m_localBid.push_back(newPriceLevel);
    }
}

void Stock::askInsert(const Quote& newQuote)
{
    bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, newQuote.getPrice(), newQuote.getSize(),
        globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisPriceLevel = m_localAsk.begin();

    if (!m_localAsk.empty()) {
        while (m_thisPriceLevel != m_localAsk.end() && m_thisPriceLevel->getPrice() <= newQuote.getPrice()) {
            if (m_thisPriceLevel->getPrice() == newQuote.getPrice()) {
                m_thisPriceLevel->push_back(newQuote);
                m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newQuote.getSize());
                break;
            }
            m_thisPriceLevel++;
        }

        if (m_thisPriceLevel == m_localAsk.end()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newQuote.getPrice());
            newPriceLevel.setSize(newQuote.getSize());

            newPriceLevel.push_front(newQuote);
            m_localAsk.push_back(newPriceLevel);
        } else if (m_thisPriceLevel->getPrice() > newQuote.getPrice()) {
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newQuote.getPrice());
            newPriceLevel.setSize(newQuote.getSize());

            newPriceLevel.push_front(newQuote);
            m_localAsk.insert(m_thisPriceLevel, newPriceLevel);
        }
    } else {
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newQuote.getPrice());
        newPriceLevel.setSize(newQuote.getSize());

        newPriceLevel.push_front(newQuote);
        m_localAsk.push_back(newPriceLevel);
    }
}

void Stock::bookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time)
{
    orderBookUpdates.push_back({ type, symbol, price, size, time });
}

void Stock::bookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time)
{
    orderBookUpdates.push_back({ type, symbol, price, size, destination, time });
}

void Stock::showLocal()
{
    long simulationMilli = globalTimeSetting.pastMilli(true);

    std::string newLevel;
    std::stringstream ss;

    ss << simulationMilli << ";"
       << "Local: "
       << ";";

    unsigned int depth = m_localAsk.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisPriceLevel = m_localAsk.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPriceLevel->getPrice() << "," << m_thisPriceLevel->getSize() << ",";
        m_thisPriceLevel++;
    }
    ss << ";";

    depth = m_localBid.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisPriceLevel = m_localBid.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPriceLevel->getPrice() << "," << m_thisPriceLevel->getSize() << ",";
        m_thisPriceLevel++;
    }
    ss << ";";

    newLevel = ss.str();
}

void Stock::showGlobal()
{
    long simulationMilli = globalTimeSetting.pastMilli(true);

    std::string newLevel;
    std::stringstream ss;

    ss << simulationMilli << ";"
       << "Global: "
       << ";";

    unsigned int depth = m_globalAsk.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisGlobal = m_globalAsk.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobal->getPrice() << "," << m_thisGlobal->getSize() << "," << m_thisGlobal->getDestination() << ",";
        m_thisGlobal++;
    }
    ss << ";";

    depth = m_globalBid.size();
    depth = m_depth > depth ? depth : m_depth;

    m_thisGlobal = m_globalBid.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobal->getPrice() << "," << m_thisGlobal->getSize() << "," << m_thisGlobal->getDestination() << ",";
        m_thisGlobal++;
    }
    ss << ";";

    newLevel = ss.str();
}

void Stock::updateGlobalBid(const Quote& newGlobal)
{
    bookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, newGlobal.getPrice(), newGlobal.getSize(),
        newGlobal.getDestination(), globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisGlobal = m_globalBid.begin();
    while (!m_globalBid.empty()) {
        if (m_thisGlobal == m_globalBid.end()) {
            m_globalBid.push_back(newGlobal);
            break;
        } else if (m_thisGlobal->getPrice() > newGlobal.getPrice()) {
            std::list<Quote>::iterator it;
            it = m_thisGlobal;
            ++m_thisGlobal;
            m_globalBid.erase(it);
        } else if (m_thisGlobal->getPrice() == newGlobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newGlobal.getDestination()) {
                m_thisGlobal->setSize(newGlobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() < newGlobal.getPrice()) {
            m_globalBid.insert(m_thisGlobal, newGlobal);
            break;
        }
    }
    if (m_globalBid.empty())
        m_globalBid.push_back(newGlobal);
}

void Stock::updateGlobalAsk(const Quote& newGlobal)
{
    bookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, newGlobal.getPrice(), newGlobal.getSize(),
        newGlobal.getDestination(), globalTimeSetting.simulationTimestamp()); // update the new price for broadcasting
    m_thisGlobal = m_globalAsk.begin();
    while (!m_globalAsk.empty()) {
        if (m_thisGlobal == m_globalAsk.end()) {
            m_globalAsk.push_back(newGlobal);
            break;
        } else if (m_thisGlobal->getPrice() < newGlobal.getPrice()) {
            std::list<Quote>::iterator it;
            it = m_thisGlobal;
            ++m_thisGlobal;
            m_globalAsk.erase(it);
        } else if (m_thisGlobal->getPrice() == newGlobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newGlobal.getDestination()) {
                m_thisGlobal->setSize(newGlobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() > newGlobal.getPrice()) {
            m_globalAsk.insert(m_thisGlobal, newGlobal);
            break;
        }
    }
    if (m_globalAsk.empty())
        m_globalAsk.push_back(newGlobal);
}

void Stock::checkGlobalBid(Quote& newQuote, const std::string& type)
{
    if (type == "limit") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid limit." << endl;
                break;
            } else if (m_thisGlobal->getPrice() >= newQuote.getPrice()) {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());

                bookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                if (size <= 0) {
                    m_globalBid.pop_front();
                }
            } else if (m_thisGlobal->getPrice() < newQuote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid market." << endl;
                break;
            } else {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());

                if (size <= 0) {
                    m_globalBid.pop_front();
                }
            }
        }
    }
}

void Stock::checkGlobalAsk(Quote& newQuote, const std::string& type)
{
    if (type == "limit") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in checkglobalask market." << endl;
                break;
            } else if (m_thisGlobal->getPrice() <= newQuote.getPrice()) {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                if (size <= 0) {
                    m_globalAsk.pop_front();
                }
            } else if (m_thisGlobal->getPrice() > newQuote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in check global ask market." << endl;
                break;
            } else {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                if (size <= 0) {
                    m_globalAsk.pop_front();
                }
            }
        }
    }
}

void Stock::doLocalLimitBuy(Quote& newQuote, const std::string& destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0;

    // init
    m_thisPriceLevel = m_localAsk.begin();
    if (m_thisPriceLevel != m_localAsk.end()) {
        m_thisQuote = m_thisPriceLevel->begin();
    }
    while (newQuote.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsk.end()) {
            if (m_thisQuote == m_thisPriceLevel->end()) {
                // if m_thisQuote reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisQuote = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestAsk = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalAsk.begin();
        if (m_thisGlobal != m_globalAsk.end()) {
            globalBestAsk = m_thisGlobal->getPrice();
        } else {
            cout << "Global ask book is empty in dolimitbuy." << endl;
        }

        // stop if no order avaiable
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        // stop if newQuote price < best price
        double bestPrice = std::min(localBestAsk, globalBestAsk);
        if (newQuote.getPrice() < bestPrice) {
            break;
        }

        // convert to market buy order
        newQuote.setOrderType('3');

        // compare the price
        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }

            // update the new price for broadcasting
            bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsk.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsk.end()) {
                    m_thisQuote = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsk.pop_front();
            }
        }

        // recover to limit buy order
        newQuote.setOrderType('1');
    }
}

void Stock::doLocalLimitSell(Quote& newQuote, const std::string& destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0;

    // init
    m_thisPriceLevel = m_localBid.begin();
    if (m_thisPriceLevel != m_localBid.end()) {
        m_thisQuote = m_thisPriceLevel->begin();
    }
    while (newQuote.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBid.end()) {
            if (m_thisQuote == m_thisPriceLevel->end()) {
                // if m_thisQuote reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisQuote = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestBid = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalBid.begin();
        if (m_thisGlobal != m_globalBid.end()) {
            globalBestBid = m_thisGlobal->getPrice();
        } else {
            cout << "Global bid book is empty in dolimitsell." << endl;
        }

        // stop if newQuote price < best price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        // stop if newQuote price > best price
        double bestPrice = std::max(localBestBid, globalBestBid);
        if (newQuote.getPrice() > bestPrice) {
            break;
        }

        // convert to market order
        newQuote.setOrderType('4');

        // compare the price
        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }

            // update the new price for broadcasting
            bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBid.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBid.end()) {
                    m_thisQuote = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBid.pop_front();
            }
        }

        // recover to limit order
        newQuote.setOrderType('2');
    }
}

void Stock::doLocalMarketBuy(Quote& newQuote, const std::string& destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0;

    // init
    m_thisPriceLevel = m_localAsk.begin();
    if (m_thisPriceLevel != m_localAsk.end()) {
        m_thisQuote = m_thisPriceLevel->begin();
    }
    while (newQuote.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localAsk.end()) {
            if (m_thisQuote == m_thisPriceLevel->end()) {
                // if m_thisQuote reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisQuote = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestAsk = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalAsk.begin();
        if (m_thisGlobal != m_globalAsk.end()) {
            globalBestAsk = m_thisGlobal->getPrice();
        } else {
            cout << "Global ask book is empty in domarketbuy." << endl;
        }

        // compare the price
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes.
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }

            // update the new price for broadcasting
            bookUpdate(OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localAsk.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localAsk.end()) {
                    m_thisQuote = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate(OrderBookEntry::Type::GLB_ASK, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsk.pop_front();
            }
        }
    }
}

void Stock::doLocalMarketSell(Quote& newQuote, const std::string& destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0;

    // init
    m_thisPriceLevel = m_localBid.begin();
    if (m_thisPriceLevel != m_localBid.end()) {
        m_thisQuote = m_thisPriceLevel->begin();
    }
    while (newQuote.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPriceLevel != m_localBid.end()) {
            if (m_thisQuote == m_thisPriceLevel->end()) {
                // if m_thisQuote reach end(), go to next m_thisPriceLevel
                ++m_thisPriceLevel;
                m_thisQuote = m_thisPriceLevel->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestBid = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalBid.begin();
        if (m_thisGlobal != m_globalBid.end()) {
            globalBestBid = m_thisGlobal->getPrice();
        } else {
            cout << "Global bid book is empty in domarketsell." << endl;
        }

        // compare the price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); // compare the two sizes
            execute(size, newQuote, '2', destination);
            if (size <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPriceLevel->erase(m_thisQuote);
            }

            // update the new price for broadcasting
            bookUpdate(OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPriceLevel->empty()) {
                m_thisPriceLevel = m_localBid.erase(m_thisPriceLevel);
                if (m_thisPriceLevel != m_localBid.end()) {
                    m_thisQuote = m_thisPriceLevel->begin();
                }
            }
        } else {
            // trade with global order
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate(OrderBookEntry::Type::GLB_BID, m_symbol, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBid.pop_front();
            }
        }
    }
}
