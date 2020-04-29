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

    OrderBook(std::string symbol, Type type);
    virtual ~OrderBook() = default;

    auto getSymbol() const -> const std::string&;
    auto getType() const -> Type;

    auto getBestValues() -> std::pair<double, int>;
    auto getBestPrice() -> double;
    auto getBestSize() -> int;
    auto getOrderBook(int maxLevel) -> std::vector<shift::OrderBookEntry>;
    auto getOrderBookWithDestination() -> std::vector<shift::OrderBookEntry>;

    void setOrderBook(std::list<shift::OrderBookEntry>&& entries);
    void resetOrderBook();
    void displayOrderBook();

    virtual void update(shift::OrderBookEntry&& entry) = 0;

protected:
    auto findEntry(double price, const std::string& destination) -> std::list<shift::OrderBookEntry>::iterator;

    std::string m_symbol;
    Type m_type;

    mutable std::mutex m_mutex;
    std::list<shift::OrderBookEntry> m_entries;
};

} // shift
