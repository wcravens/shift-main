#include "OrderBookLocalBid.h"

/**
 * @brief Constructor with all members preset.
 * @param symbol string value to be set in m_symbol
 */
shift::OrderBookLocalBid::OrderBookLocalBid(const std::string& symbol)
    : OrderBook(symbol, shift::OrderBook::Type::LOCAL_BID)
{
}

/**
 * @brief Method to update the current orderbook.
 * @param entry The entry to be inserted into/updated from the order book.
 */
void shift::OrderBookLocalBid::update(shift::OrderBookEntry&& entry)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto it = m_entries.begin();

    for (; it != m_entries.end(); it++) {
        if (it->getPrice() < entry.getPrice())
            break;

        if (it->getPrice() == entry.getPrice()) {
            if (!entry.getSize()) {
                it = m_entries.erase(it);
            } else {
                it->setSize(entry.getSize());
            }

            return;
        }
    }
    m_entries.insert(it, std::move(entry));
}
