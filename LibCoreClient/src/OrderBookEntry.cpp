#include "OrderBookEntry.h"

namespace shift {

OrderBookEntry::OrderBookEntry()
    : OrderBookEntry { 0.0, 0, "", std::chrono::system_clock::time_point() }
{
}

OrderBookEntry::OrderBookEntry(double price, int size, std::string destination, std::chrono::system_clock::time_point time)
    : m_price { price }
    , m_size { size }
    , m_destination { std::move(destination) }
    , m_time { std::move(time) }
{
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

auto OrderBookEntry::getTime() const -> const std::chrono::system_clock::time_point&
{
    return m_time;
}

void OrderBookEntry::setPrice(double price)
{
    m_price = price;
}

void OrderBookEntry::setSize(int size)
{
    m_size = size;
}

void OrderBookEntry::setDestination(const std::string& destination)
{
    m_destination = destination;
}

void OrderBookEntry::setTime(const std::chrono::system_clock::time_point& time)
{
    m_time = time;
}

} // shift
