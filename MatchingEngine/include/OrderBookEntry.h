#pragma once

#include <list>
#include <string>

#include <quickfix/FieldTypes.h>

class OrderBookEntry {
public:
    enum Type : char {
        GLB_ASK = 'A',
        GLB_BID = 'B',
        LOC_ASK = 'a',
        LOC_BID = 'b',
        OTHER = 'e'
    };

    OrderBookEntry(Type type, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp realTime);
    OrderBookEntry(Type type, std::string symbol, double price, int size, FIX::UtcTimeStamp realTime);

    // getters
    auto getType() const -> Type;
    auto getSymbol() const -> const std::string&;
    auto getPrice() const -> double;
    auto getSize() const -> int;
    auto getDestination() const -> const std::string&;
    auto getUTCTime() const -> const FIX::UtcTimeStamp&;

private:
    Type m_type;
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_realTime;
};
