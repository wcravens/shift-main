#pragma once

#include <string>

#include <quickfix/FieldTypes.h>

/**
 * @brief Class that maintains an instance of list of orders, and provides options-contract related controls.
 */
class OrderBookEntry {
public:
    enum Type : char {
        GLB_ASK = 'A',
        GLB_BID = 'B',
        LOC_ASK = 'a',
        LOC_BID = 'b',
        OTHER = 'e'
    };

    OrderBookEntry() = default;
    OrderBookEntry(OrderBookEntry::Type type, std::string symbol, double price, int size, std::string destination, FIX::UtcDateOnly simulationDate, FIX::UtcTimeOnly simulationTime);

    auto getType() const -> Type;
    auto getSymbol() const -> const std::string&;
    auto getPrice() const -> double;
    auto getSize() const -> int;
    auto getDestination() const -> const std::string&;
    auto getDate() const -> const FIX::UtcDateOnly&;
    auto getTime() const -> const FIX::UtcTimeOnly&;

private:
    Type m_type;
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcDateOnly m_simulationDate;
    FIX::UtcTimeOnly m_simulationTime;
};
