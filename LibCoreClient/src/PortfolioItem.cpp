#include "PortfolioItem.h"

namespace shift {

PortfolioItem::PortfolioItem(std::string symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL)
    : m_symbol { std::move(symbol) }
    , m_longShares { longShares }
    , m_shortShares { shortShares }
    , m_longPrice { longPrice }
    , m_shortPrice { shortPrice }
    , m_realizedPL { realizedPL }
    , m_timestamp { std::chrono::system_clock::now() }
{
}

auto PortfolioItem::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto PortfolioItem::getShares() const -> int
{
    return m_longShares - m_shortShares;
}

auto PortfolioItem::getLongShares() const -> int
{
    return m_longShares;
}

auto PortfolioItem::getShortShares() const -> int
{
    return m_shortShares;
}

auto PortfolioItem::getPrice() const -> double
{
    int shares = getShares();

    if (shares > 0) {
        return m_longPrice;
    }

    if (shares < 0) {
        return m_shortPrice;
    }

    return 0.0;
}

auto PortfolioItem::getLongPrice() const -> double
{
    return m_longPrice;
}

auto PortfolioItem::getShortPrice() const -> double
{
    return m_shortPrice;
}

auto PortfolioItem::getRealizedPL() const -> double
{
    return m_realizedPL;
}

auto PortfolioItem::getTimestamp() const -> const std::chrono::system_clock::time_point&
{
    return m_timestamp;
}

} // shift
