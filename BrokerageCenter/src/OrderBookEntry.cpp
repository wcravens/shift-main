#include "OrderBookEntry.h"

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcDateOnly& simulationDate, const FIX::UtcTimeOnly& simulationTime)
    : m_type(type)
    , m_symbol(symbol)
    , m_price(price)
    , m_size(size)
    , m_destination(destination)
    , m_simulationDate(simulationDate)
    , m_simulationTime(simulationTime)
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

const FIX::UtcDateOnly& OrderBookEntry::getDate() const
{
    return m_simulationDate;
}

const FIX::UtcTimeOnly& OrderBookEntry::getTime() const
{
    return m_simulationTime;
}
