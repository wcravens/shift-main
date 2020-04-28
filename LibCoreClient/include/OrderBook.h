#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBookEntry.h"

#include <iostream>
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
        GLOBAL_ASK = 'A',
        LOCAL_BID = 'b',
        LOCAL_ASK = 'a'
    };

    OrderBook(const std::string& symbol, Type type);
    virtual ~OrderBook() = default;

    const std::string& getSymbol() const;
    Type getType() const;

    double getBestPrice();
    int getBestSize();
    std::vector<shift::OrderBookEntry> getOrderBook(int maxLevel);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestination();

    void setOrderBook(std::list<shift::OrderBookEntry>&& entries);
    void resetOrderBook();
    void displayOrderBook();

    virtual void update(shift::OrderBookEntry&& entry) = 0;

protected:
    std::list<shift::OrderBookEntry>::iterator findEntry(double price, const std::string& destination);

    std::string m_symbol;
    Type m_type;

    mutable std::mutex m_mutex;
    std::list<shift::OrderBookEntry> m_entries;
};

} // shift
