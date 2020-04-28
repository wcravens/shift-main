#include "OrderBookEntry.h"

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, std::string symbol, double price, int size, std::string destination, FIX::UtcDateOnly simulationDate, FIX::UtcTimeOnly simulationTime)
    : m_type { type }
    , m_symbol { std::move(symbol) }
    , m_price { price }
    , m_size { size }
    , m_destination { std::move(destination) }
    , m_simulationDate { std::move(simulationDate) }
    , m_simulationTime { std::move(simulationTime) }
{
}

auto OrderBookEntry::getType() const -> OrderBookEntry::Type
{
    return m_type;
}

auto OrderBookEntry::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto OrderBookEntry::getPrice() const -> double
{
    return m_price;
}

auto OrderBookEntry::getSize() const -> int
{
    return m_size;
}

auto OrderBookEntry::getDestination() const -> const std::string&
{
    return m_destination;
}

auto OrderBookEntry::getDate() const -> const FIX::UtcDateOnly&
{
    return m_simulationDate;
}

auto OrderBookEntry::getTime() const -> const FIX::UtcTimeOnly&
{
    return m_simulationTime;
}
