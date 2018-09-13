#include "PortfolioItem.h"

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

double shift::PortfolioItem::getPL() const
{
    return m_pl;
}
