#include "Price.h"

double Price::getPrice() const
{
    return m_price;
}

int Price::getSize() const
{
    return m_size;
}

void Price::setPrice(double price)
{
    m_price = price;
}

void Price::setSize(int size)
{
    m_size = size;
}

bool Price::empty()
{
    return m_quotes.empty();
}

void Price::push_back(const Quote& quote)
{
    m_quotes.push_back(quote);
}

void Price::push_front(const Quote& quote)
{
    m_quotes.push_front(quote);
}

std::list<Quote>::iterator Price::begin()
{
    return m_quotes.begin();
}

std::list<Quote>::iterator Price::end()
{
    return m_quotes.end();
}

std::list<Quote>::iterator Price::erase(std::list<Quote>::iterator iter)
{
    return m_quotes.erase(iter);
}
