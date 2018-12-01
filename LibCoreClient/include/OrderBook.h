#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBookEntry.h"

#include <list>
#include <mutex>
#include <string>
#include <vector>

namespace shift {

/**
 * @brief A class for Order Book object, has a list of entries as member data.
 */
class CORECLIENT_EXPORTS OrderBook {
public:
    enum class Type {
        GLOBAL_BID = 'B',
        LOCAL_BID = 'b',
        GLOBAL_ASK = 'A',
        LOCAL_ASK = 'a'
    };

    OrderBook();
    virtual ~OrderBook();

    double getBestPrice();
    int getBestSize();
    std::vector<shift::OrderBookEntry> getOrderBook();
    std::vector<shift::OrderBookEntry> getOrderBook2(int maxLevel);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestination();

    void setOrderBook(std::vector<shift::OrderBookEntry> entries);

    virtual void update(shift::OrderBookEntry entry) = 0;

protected:
    std::list<shift::OrderBookEntry>::iterator findEntryByDestPrice(std::string dest, double price);

    std::mutex m_mutex; //!< Mutex member to lock the list when it's being adjusted.
    std::list<shift::OrderBookEntry> m_entries; //!< A list of all entries within the current OrderBook object.

    Type m_type;
};

} // shift
