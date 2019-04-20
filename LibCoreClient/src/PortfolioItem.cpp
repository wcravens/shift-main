#include "PortfolioItem.h"

shift::PortfolioItem::PortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL)
    : m_symbol(symbol)
    , m_longShares(longShares)
    , m_shortShares(shortShares)
    , m_longPrice(longPrice)
    , m_shortPrice(shortPrice)
    , m_realizedPL(realizedPL)
    , m_timestamp(std::chrono::system_clock::now())
{
}

const std::string& shift::PortfolioItem::getSymbol() const
{
    return m_symbol;
}

int shift::PortfolioItem::getShares() const
{
    return m_longShares - m_shortShares;
}

int shift::PortfolioItem::getLongShares() const
{
    return m_longShares;
}

int shift::PortfolioItem::getShortShares() const
{
    return m_shortShares;
}

double shift::PortfolioItem::getPrice() const
{
    if (getShares() == 0) {
        return 0.0;
    } else if (getShares() > 0) {
        return m_longPrice;
    } else {
        return m_shortPrice;
    }
}

double shift::PortfolioItem::getLongPrice() const
{
    return m_longPrice;
}

double shift::PortfolioItem::getShortPrice() const
{
    return m_shortPrice;
}

double shift::PortfolioItem::getRealizedPL() const
{
    return m_realizedPL;
}

const std::chrono::system_clock::time_point& shift::PortfolioItem::getTimestamp() const
{
    return m_timestamp;
}
