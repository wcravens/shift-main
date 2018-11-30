#include "PortfolioItem.h"

shift::PortfolioItem::PortfolioItem(const std::string& symbol)
    : m_symbol(symbol)
    , m_shares(0.0)
    , m_price(0.0)
    , m_realizedPL(0.0)
{
}

shift::PortfolioItem::PortfolioItem(const std::string& symbol, int shares, double price, double realizedPL)
    : m_symbol(symbol)
    , m_shares(shares)
    , m_price(price)
    , m_realizedPL(realizedPL)
{
}

const std::string& shift::PortfolioItem::getSymbol() const
{
    return m_symbol;
}

int shift::PortfolioItem::getShares() const
{
    return m_shares;
}

double shift::PortfolioItem::getPrice() const
{
    return m_price;
}

double shift::PortfolioItem::getRealizedPL() const
{
    return m_realizedPL;
}
