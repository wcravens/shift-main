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

    OrderBookEntry(Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);
    OrderBookEntry(Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);

    // Getters
    Type getType() const;
    const std::string& getSymbol() const;
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    const FIX::UtcTimeStamp& getUTCTime() const;

private:
    Type m_type;
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
