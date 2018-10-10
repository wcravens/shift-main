#include "OrderBook.h"

/**
 * @brief Default constructor.
 */
shift::OrderBook::OrderBook()
{
}

/**
 * @brief Default destructor.
 */
shift::OrderBook::~OrderBook()
{
}

/**
 * @brief Method to get the current best price.
 * @return double value of the current best price.
 */
double shift::OrderBook::getBestPrice()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_entries.empty())
        return m_entries.begin()->getPrice();

    return 0;
}

/**
 * @brief Method to get the size of corresponding best price entry.
 * @return int as the size of the corresponding best price entry.
 */
int shift::OrderBook::getBestSize()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    int bestSize = 0;
    double bestPrice = 0.0;

    if (!m_entries.empty()) {
        bestPrice = m_entries.begin()->getPrice();

        // Add up the size of all entries with the same price.
        for (shift::OrderBookEntry entry : m_entries) {
            if (entry.getPrice() == bestPrice)
                bestSize += entry.getSize();
            else
                return bestSize;
        }
    }
    return bestSize;
}

/**
 * @brief Method to return up to the top 5 orders from the current orderbook.
 * @return A vector contains up to 5 orders from the current orderbook.
 */
std::vector<shift::OrderBookEntry> shift::OrderBook::getOrderBook()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<shift::OrderBookEntry> orderBook;

    if (!m_entries.empty()) {
        int level = 0;
        shift::OrderBookEntry lastEntry = *m_entries.begin();
        lastEntry.setPrice(-1.0);
        lastEntry.setSize(-1);

        if (m_type == OrderBook::Type::GLOBAL_ASK || m_type == OrderBook::Type::GLOBAL_BID)
            lastEntry.setDestination("Market");

        for (shift::OrderBookEntry entry : m_entries) {
            if (lastEntry.getPrice() == entry.getPrice())
                lastEntry.setSize(lastEntry.getSize() + entry.getSize());
            else {
                if (lastEntry.getPrice() > 0.0)
                    orderBook.push_back(lastEntry);
                lastEntry.setSize(entry.getSize());
                lastEntry.setPrice(entry.getPrice());

                // Display at most 5 levels of order book.
                if (++level > 5)
                    return orderBook;
            }
        }
        if (lastEntry.getPrice() > 0.0)
            orderBook.push_back(lastEntry);
    }

    return orderBook;
}

/**
 * @brief Method to return the designated order book searched by destination.
 * @return the target order book with their own destination. (Not combined to "Market")
 */
std::vector<shift::OrderBookEntry> shift::OrderBook::getOrderBookWithDestination()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<shift::OrderBookEntry> orderBook;
    double lastPrice = -1.0;
    int level = 0;

    for (shift::OrderBookEntry entry : m_entries) {
        if (entry.getPrice() != lastPrice) {
            lastPrice = entry.getPrice();
            if (++level > 5)
                return orderBook;
        }
        orderBook.push_back(entry);
    }

    return orderBook;
}

/**
 * @brief Method to set the input vector of entries into m_entries of current order book.
 * @param entries A vector of shift::OrderBookEntry including all entries to be inserted.
 */
void shift::OrderBook::setOrderBook(std::vector<shift::OrderBookEntry> entries)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries.clear();
    for (shift::OrderBookEntry entry : entries)
        if (entry.getSize())
            m_entries.push_back(entry);
}

/**
 * @brief Method to return target position of the order book entry who has the requested destination and price.
 * @param dest the target destination as a string
 * @param price the target price value as a double
 * @return A list iterator who points to the position of the requested order book entry.
 */
std::list<shift::OrderBookEntry>::iterator shift::OrderBook::findEntryByDestPrice(std::string dest, double price)
{
    for (std::list<shift::OrderBookEntry>::iterator it = m_entries.begin(); it != m_entries.end(); it++) {
        if (it->getPrice() != price)
            break;

        if (it->getDestination() == dest)
            return it;
    }
    return m_entries.end();
}
