#include "OrderBook.h"

/**
 * @brief Constructor with all members preset.
 * @param symbol String value to be set in m_symbol.
 * @param type Type value for m_type.
 */
shift::OrderBook::OrderBook(const std::string& symbol, shift::OrderBook::Type type)
    : m_symbol(symbol)
    , m_type(type)
{
}

const std::string& shift::OrderBook::getSymbol() const
{
    return m_symbol;
}

shift::OrderBook::Type shift::OrderBook::getType() const
{
    return m_type;
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

    return 0.0;
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
                if (m_type == shift::OrderBook::Type::GLOBAL_ASK || m_type == shift::OrderBook::Type::GLOBAL_BID) {
                    newEntry.setDestination("Market");
                }
            }
        }
        output.push_back(newEntry);
    }

    return output;
}

/**
 * @brief Method to return the designated order book searched by destination.
 * @return the target order book with their own destination (not combined to "Market"). 
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
void shift::OrderBook::setOrderBook(std::list<shift::OrderBookEntry>&& entries)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries = std::move(entries);
}

/**
 * @brief Method to reset the m_entries of current order book (clear it).
 */
void shift::OrderBook::resetOrderBook()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries.clear();
}

/**
 * @brief Method to display the current order book (for debugging).
 */
void shift::OrderBook::displayOrderBook()
{
    std::cout << std::endl
              << static_cast<char>(m_type) << ':' << std::endl;

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        std::cout << it->getPrice() << '\t' << it->getSize() << '\t' << it->getDestination() << std::endl;
    }

    std::cout << std::endl;
}

/**
 * @brief Method to return target position of the order book entry who has the requested price and destination.
 * @param price The target price value as a double.
 * @param destination The target destination as a string.
 * @return A list iterator who points to the position of the requested order book entry.
 */
std::list<shift::OrderBookEntry>::iterator shift::OrderBook::findEntry(double price, const std::string& destination)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->getPrice() != price) {
            break;
        }

        if (it->getDestination() == destination) {
            return it;
        }
    }

    return m_entries.end();
}
