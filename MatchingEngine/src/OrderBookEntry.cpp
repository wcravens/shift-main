#include "OrderBookEntry.h"

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp realTime)
    : m_type { type }
    , m_symbol { std::move(symbol) }
    , m_price { price }
    , m_size { size }
    , m_destination { std::move(destination) }
    , m_realTime { std::move(realTime) }
{
}

OrderBookEntry::OrderBookEntry(OrderBookEntry::Type type, std::string symbol, double price, int size, FIX::UtcTimeStamp realTime)
    : m_type { type }
    , m_symbol { std::move(symbol) }
    , m_price { price }
    , m_size { size }
    , m_destination { "SHIFT" }
    , m_realTime { std::move(realTime) }
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

auto OrderBookEntry::getUTCTime() const -> const FIX::UtcTimeStamp&
{
    return m_realTime;
}
