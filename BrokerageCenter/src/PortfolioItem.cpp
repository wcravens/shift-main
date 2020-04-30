#include "PortfolioItem.h"

PortfolioItem::PortfolioItem()
    : PortfolioItem { "", 0.0, 0.0, 0.0, 0.0, 0, 0 }
{
}

PortfolioItem::PortfolioItem(std::string symbol)
    : PortfolioItem { std::move(symbol), 0.0, 0.0, 0.0, 0.0, 0, 0 }
{
}

PortfolioItem::PortfolioItem(std::string symbol, int shares, double price)
    : m_symbol { std::move(symbol) }
    , m_borrowedBalance { 0.0 }
    , m_pl { 0.0 }
{
    if (shares >= 0) {
        m_longPrice = price;
        m_shortPrice = 0.0;
        m_longShares = shares;
        m_shortShares = 0;
    } else {
        m_longPrice = 0.0;
        m_shortPrice = price;
        m_longShares = 0;
        m_shortShares = -shares;
    }
}

PortfolioItem::PortfolioItem(std::string symbol, double borrowedBalace, double pl, double longPrice, double shortPrice, int longShares, int shortShares)
    : m_symbol { std::move(symbol) }
    , m_borrowedBalance { borrowedBalace }
    , m_pl { pl }
    , m_longPrice { longPrice }
    , m_shortPrice { shortPrice }
    , m_longShares { longShares }
    , m_shortShares { shortShares }
{
}

void PortfolioItem::addLongShares(int value)
{
    m_longShares += value;
}

void PortfolioItem::addShortShares(int value)
{
    m_shortShares += value;
}

void PortfolioItem::addLongPrice(double value, int quantity)
{
    if (quantity > 0) {
        m_longPrice = (m_longPrice * m_longShares + value * quantity) / (m_longShares + quantity);
    }
}

void PortfolioItem::addShortPrice(double value, int quantity)
{
    if (quantity > 0) {
        m_shortPrice = (m_shortPrice * m_shortShares + value * quantity) / (m_shortShares + quantity);
    }
}

void PortfolioItem::addPL(double value)
{
    m_pl += value;
}

void PortfolioItem::addBorrowedBalance(double value)
{
    m_borrowedBalance += value;
}

void PortfolioItem::resetLongShares()
{
    m_longShares = 0;
}

void PortfolioItem::resetShortShares()
{
    m_shortShares = 0;
}

void PortfolioItem::resetLongPrice()
{
    m_longPrice = 0.0;
}

void PortfolioItem::resetShortPrice()
{
    m_shortPrice = 0.0;
}

void PortfolioItem::resetPL()
{
    m_pl = 0.0;
}

void PortfolioItem::resetBorrowedBalance()
{
    m_borrowedBalance = 0.0;
}

auto PortfolioItem::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto PortfolioItem::getSummaryShares() const -> int
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

auto PortfolioItem::getSummaryPrice() const -> double
{
    int shares = getSummaryShares();

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

auto PortfolioItem::getPL() const -> double
{
    return m_pl;
}

auto PortfolioItem::getBorrowedBalance() const -> double
{
    return m_borrowedBalance;
}
