#include "OrderBookLocalAsk.h"

#include <cmath>
#include <limits>

shift::OrderBookLocalAsk::OrderBookLocalAsk(const std::string& symbol)
    : OrderBook(symbol, shift::OrderBook::Type::LOCAL_ASK)
{
}

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

        if (std::fabs(it->getPrice() - entry.getPrice()) < std::numeric_limits<double>::epsilon()) {
            if (!entry.getSize())
                it = m_entries.erase(it);
            else
                it->setSize(entry.getSize());

            return;
        }
    }
    m_entries.insert(it, entry);
}
