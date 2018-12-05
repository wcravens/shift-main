#include "OrderBookGlobalAsk.h"

/**
 * @brief Constructor with all members preset.
 * @param symbol string value to be set in m_symbol
 */
shift::OrderBookGlobalAsk::OrderBookGlobalAsk(const std::string& symbol)
    : OrderBook(symbol, shift::OrderBook::Type::GLOBAL_ASK)
{
}

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
void shift::OrderBookGlobalAsk::update(const shift::OrderBookEntry& entry)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->getPrice() < entry.getPrice())
            // remove price less than current price
            it = m_entries.erase(it);
        else {
            // check if the destination exist
            auto it_dest = findEntry(entry.getPrice(), entry.getDestination());
            if (it_dest != m_entries.end()) {
                *it = entry;
                return;
            } else {
                break;
            }
        }
    }
    // Insert the new order into the front of the order book.
    m_entries.insert(m_entries.begin(), entry);
}
