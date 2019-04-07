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
    return m_quotes.empty();
}

void PriceLevel::push_back(const Quote& quote)
{
    m_quotes.push_back(quote);
}

void PriceLevel::push_front(const Quote& quote)
{
    m_quotes.push_front(quote);
}

std::list<Quote>::iterator PriceLevel::begin()
{
    return m_quotes.begin();
}

std::list<Quote>::iterator PriceLevel::end()
{
    return m_quotes.end();
}

std::list<Quote>::iterator PriceLevel::erase(std::list<Quote>::iterator iter)
{
    return m_quotes.erase(iter);
}
