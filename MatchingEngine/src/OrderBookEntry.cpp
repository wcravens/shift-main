#include "OrderBookEntry.h"

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& realTime)
    : m_type(type)
    , m_symbol(symbol)
    , m_price(price)
    , m_size(size)
    , m_destination(destination)
    , m_realTime(realTime)
{
}

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& realTime)
    : m_type(type)
    , m_symbol(symbol)
    , m_price(price)
    , m_size(size)
    , m_destination("SHIFT")
    , m_realTime(realTime)
{
}

OrderBookEntry::Type OrderBookEntry::getType() const
{
    return m_type;
}

const std::string& OrderBookEntry::getSymbol() const
{
    return m_symbol;
}

double OrderBookEntry::getPrice() const
{
    return m_price;
}

int OrderBookEntry::getSize() const
{
    return m_size;
}

const std::string& OrderBookEntry::getDestination() const
{
    return m_destination;
}

const FIX::UtcTimeStamp& OrderBookEntry::getUTCTime() const
{
    return m_realTime;
}
