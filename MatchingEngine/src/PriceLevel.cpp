#include "PriceLevel.h"

double PriceLevel::getPrice() const
{
    return m_price;
}

int PriceLevel::getSize() const
{
    return m_size;
}

void PriceLevel::setPrice(double price)
{
    m_price = price;
}

void PriceLevel::setSize(int size)
{
    m_size = size;
}

bool PriceLevel::empty()
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

std::list<Order>::iterator PriceLevel::begin()
{
    return m_orders.begin();
}

std::list<Order>::iterator PriceLevel::end()
{
    return m_orders.end();
}

std::list<Order>::iterator PriceLevel::erase(std::list<Order>::iterator iter)
{
    return m_orders.erase(iter);
}
