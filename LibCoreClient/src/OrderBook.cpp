#include "OrderBook.h"

namespace shift {

/**
 * @brief Constructor with all members preset.
 * @param symbol String value to be set in m_symbol.
 * @param type Type value for m_type.
 */
OrderBook::OrderBook(std::string symbol, OrderBook::Type type)
    : m_symbol { std::move(symbol) }
    , m_type { type }
{
}

auto OrderBook::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto OrderBook::getType() const -> OrderBook::Type
{
    return m_type;
}

/**
 * @brief Method to get the current best price.
 * @return double value of the current best price.
 */
auto OrderBook::getBestPrice() -> double
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_entries.empty()) {
        return m_entries.begin()->getPrice();
    }

    return 0.0;
}

/**
 * @brief Method to get the size of corresponding best price entry.
 * @return int as the size of the corresponding best price entry.
 */
auto OrderBook::getBestSize() -> int
{
    std::lock_guard<std::mutex> guard(m_mutex);
    int bestSize = 0;
    double bestPrice = 0.0;

    if (!m_entries.empty()) {
        bestPrice = m_entries.begin()->getPrice();

        // Add up the size of all entries with the same price.
        for (OrderBookEntry entry : m_entries) {
            if (entry.getPrice() == bestPrice) {
                bestSize += entry.getSize();
            } else {
                return bestSize;
            }
        }
    }
    return bestSize;
}

/**
 * @brief Method to return up to the top maxLevel orders from the current orderbook.
 * @return A vector contains up to maxLevel orders from the current orderbook.
 */
auto OrderBook::getOrderBook(int maxLevel) -> std::vector<OrderBookEntry>
{
    std::list<OrderBookEntry> original;
    std::vector<OrderBookEntry> output;
    OrderBookEntry newEntry;

    if (maxLevel <= 0) {
        return output;
    }

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        original = m_entries;
    }

    if (!original.empty()) {
        for (OrderBookEntry entry : original) {
            if (newEntry.getPrice() == entry.getPrice()) {
                newEntry.setSize(newEntry.getSize() + entry.getSize());
            } else {
                if (newEntry.getSize() > 0) {
                    output.push_back(newEntry);
                    if (--maxLevel == 0) {
                        return output;
                    }
                }
                newEntry = entry;
                if (m_type == OrderBook::Type::GLOBAL_ASK || m_type == OrderBook::Type::GLOBAL_BID) {
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
auto OrderBook::getOrderBookWithDestination() -> std::vector<OrderBookEntry>
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::vector<OrderBookEntry> output(m_entries.size());

    std::copy(m_entries.begin(), m_entries.end(), output.begin());

    return output;
}

/**
 * @brief Method to set the input list of entries into m_entries of current order book.
 * @param entries A list of OrderBookEntry including all entries to be inserted.
 */
void OrderBook::setOrderBook(std::list<OrderBookEntry>&& entries)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries = std::move(entries);
}

/**
 * @brief Method to reset the m_entries of current order book (clear it).
 */
void OrderBook::resetOrderBook()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_entries.clear();
}

/**
 * @brief Method to display the current order book (for debugging).
 */
void OrderBook::displayOrderBook()
{
    std::cout << std::endl
              << static_cast<char>(m_type) << ':' << std::endl;

    for (const auto& entry : m_entries) {
        std::cout << entry.getPrice() << '\t' << entry.getSize() << '\t' << entry.getDestination() << std::endl;
    }

    std::cout << std::endl;
}

/**
 * @brief Method to return target position of the order book entry who has the requested price and destination.
 * @param price The target price value as a double.
 * @param destination The target destination as a string.
 * @return A list iterator who points to the position of the requested order book entry.
 */
auto OrderBook::findEntry(double price, const std::string& destination) -> std::list<OrderBookEntry>::iterator
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

} // shift
