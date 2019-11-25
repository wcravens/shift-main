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
    OrderBookEntry(Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcDateOnly& simulationDate, const FIX::UtcTimeOnly& simulationTime);

    Type getType() const;
    const std::string& getSymbol() const;
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    const FIX::UtcDateOnly& getDate() const;
    const FIX::UtcTimeOnly& getTime() const;

private:
    Type m_type;
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcDateOnly m_simulationDate;
    FIX::UtcTimeOnly m_simulationTime;
};
