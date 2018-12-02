#include "OrderBookLocalAsk.h"

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
void shift::OrderBookLocalAsk::update(const shift::OrderBookEntry& entry)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto it = m_entries.begin();

    for (; it != m_entries.end(); it++) {
        if (it->getPrice() > entry.getPrice())
            break;

        if (it->getPrice() == entry.getPrice()) {
            if (!entry.getSize())
                it = m_entries.erase(it);
            else
                it->setSize(entry.getSize());

            return;
        }
    }
    m_entries.insert(it, entry);
}
