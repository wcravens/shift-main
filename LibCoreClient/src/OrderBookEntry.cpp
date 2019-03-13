#include "OrderBookEntry.h"

shift::OrderBookEntry::OrderBookEntry()
    : m_price(0.0)
    , m_size(0)
    , m_destination("")
{
}

shift::OrderBookEntry::OrderBookEntry(double price, int size, const std::string& destination, std::chrono::system_clock::time_point time)
    : m_price(price)
    , m_size(size)
    , m_destination(destination)
    , m_time(time)
{
}

double shift::OrderBookEntry::getPrice() const
{
    return m_price;
}

int shift::OrderBookEntry::getSize() const
{
    return m_size;
}

const std::chrono::system_clock::time_point shift::OrderBookEntry::getTime() const
{
    return m_time;
}

void shift::OrderBookEntry::setTime(std::chrono::system_clock::time_point time)
{
    m_time = time;
}

const std::string& shift::OrderBookEntry::getDestination() const
{
    return m_destination;
}

void shift::OrderBookEntry::setPrice(double price)
{
    m_price = price;
}

void shift::OrderBookEntry::setSize(int size)
{
    m_size = size;
}

void shift::OrderBookEntry::setDestination(const std::string& destination)
{
    m_destination = destination;
}
