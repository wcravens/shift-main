#include "OrderBookLocalBid.h"

namespace shift {

/**
 * @brief Constructor with all members preset.
 * @param symbol String value to be set in m_symbol.
 */
OrderBookLocalBid::OrderBookLocalBid(std::string symbol)
    : OrderBook { std::move(symbol), OrderBook::Type::LOCAL_BID }
{
}

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
/* virtual */ void OrderBookLocalBid::update(OrderBookEntry&& entry) // override
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto it = m_entries.begin();

    while (it != m_entries.end()) {
        if (it->getPrice() < entry.getPrice()) {
            break;
        }

        if (it->getPrice() == entry.getPrice()) {
            if (entry.getSize() > 0) {
                it->setSize(entry.getSize());
            } else {
                it = m_entries.erase(it);
            }

            return;
        }

        ++it;
    }

    m_entries.insert(it, std::move(entry));
}

} // shift
