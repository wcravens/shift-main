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
 * @brief Method to return up to the top maxLevel orders from the current orderbook.
 * @return A vector contains up to maxLevel orders from the current orderbook.
 */
std::vector<shift::OrderBookEntry> shift::OrderBook::getOrderBook(int maxLevel)
{
    std::list<shift::OrderBookEntry> original;
    std::vector<shift::OrderBookEntry> output;
    shift::OrderBookEntry newEntry;

    if (maxLevel <= 0)
        return output;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        original = m_entries;
    }

    if (!original.empty()) {
        for (shift::OrderBookEntry entry : original) {
            if (newEntry.getPrice() == entry.getPrice())
                newEntry.setSize(newEntry.getSize() + entry.getSize());
            else {
                if (newEntry.getSize() > 0) {
                    output.push_back(newEntry);
                    if (--maxLevel == 0)
                        return output;
                }
                newEntry = entry;
                if (m_type == OrderBook::Type::GLOBAL_ASK || m_type == OrderBook::Type::GLOBAL_BID)
                    newEntry.setDestination("Market");
            }
        }
        output.push_back(newEntry);
    }

    return output;
}

/**
 * @brief Method to return the designated order book searched by destination.
 * @return the target order book with their own destination. (Not combined to "Market")
 */
std::vector<shift::OrderBookEntry> shift::OrderBook::getOrderBookWithDestination()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<shift::OrderBookEntry> output(m_entries.size());

    std::copy(m_entries.begin(), m_entries.end(), output.begin());

    return output;
}

/**
 * @brief Method to set the input list of entries into m_entries of current order book.
 * @param entries A list of shift::OrderBookEntry including all entries to be inserted.
 */
void shift::OrderBook::setOrderBook(const std::list<shift::OrderBookEntry>& entries)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries = entries;
}

/**
 * @brief Method to return target position of the order book entry who has the requested destination and price.
 * @param dest the target destination as a string
 * @param price the target price value as a double
 * @return A list iterator who points to the position of the requested order book entry.
 */
std::list<shift::OrderBookEntry>::iterator shift::OrderBook::findEntryByDestPrice(const std::string& dest, double price)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); it++) {
        if (it->getPrice() != price)
            break;

        if (it->getDestination() == dest)
            return it;
    }
    return m_entries.end();
}
