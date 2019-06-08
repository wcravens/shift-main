#include "OrderBookGlobalBid.h"

/**
 * @brief Constructor with all members preset.
 * @param symbol string value to be set in m_symbol
 */
shift::OrderBookGlobalBid::OrderBookGlobalBid(const std::string& symbol)
    : OrderBook(symbol, shift::OrderBook::Type::GLOBAL_BID)
{
}

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
void shift::OrderBookGlobalBid::update(shift::OrderBookEntry&& entry)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto it = m_entries.begin();

    while (it != m_entries.end()) {
        if (it->getPrice() > entry.getPrice())
            // remove price greater than current price
            it = m_entries.erase(it);
        else {
            // check if the destination exist
            auto it_dest = findEntry(entry.getPrice(), entry.getDestination());
            if (it_dest != m_entries.end()) {
                *it_dest = std::move(entry);
                return;
            } else {
                break;
            }
        }
    }

    // insert the new order into the front of the order book
    m_entries.insert(m_entries.begin(), std::move(entry));
}
