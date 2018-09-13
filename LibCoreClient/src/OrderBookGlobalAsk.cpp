#include "OrderBookGlobalAsk.h"

/**
 * @brief Default constructor of OrderBookGlobalAsk class.
 */
shift::OrderBookGlobalAsk::OrderBookGlobalAsk()
{
}

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
void shift::OrderBookGlobalAsk::update(shift::OrderBookEntry entry)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    for (std::list<shift::OrderBookEntry>::iterator it = m_entries.begin(); it != m_entries.end();) {
        if (it->getPrice() < entry.getPrice())
            // remove price less than current price
            it = m_entries.erase(it);
        else {
            // check if the destination exist
            std::list<shift::OrderBookEntry>::iterator it_dest = findEntryByDestPrice(entry.getDestination(), entry.getPrice());
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
