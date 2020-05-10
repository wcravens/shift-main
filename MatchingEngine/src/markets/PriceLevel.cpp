#include "markets/PriceLevel.h"

namespace markets {

auto PriceLevel::getPrice() const -> double
{
    return m_price;
}

auto PriceLevel::getSize() const -> int
{
    return m_size;
}

auto PriceLevel::getNumOrders() const -> int
{
    return m_orders.size();
}

void PriceLevel::setPrice(double price)
{
    m_price = price;
}

void PriceLevel::setSize(int size)
{
    m_size = size;
}

auto PriceLevel::empty() -> bool
{
    return m_orders.empty();
}

void PriceLevel::push_back(const Order& order)
{
    m_orders.push_back(order);
}

void PriceLevel::push_front(const Order& order)
{
    m_orders.push_front(order);
}

auto PriceLevel::begin() -> std::list<Order>::iterator
{
    return m_orders.begin();
}

auto PriceLevel::begin() const -> std::list<Order>::const_iterator
{
    return m_orders.begin();
}

auto PriceLevel::end() -> std::list<Order>::iterator
{
    return m_orders.end();
}

auto PriceLevel::end() const -> std::list<Order>::const_iterator
{
    return m_orders.end();
}

auto PriceLevel::erase(std::list<Order>::iterator iter) -> std::list<Order>::iterator
{
    return m_orders.erase(iter);
}

} // markets
