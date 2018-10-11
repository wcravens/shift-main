#include "PortfolioSummary.h"

PortfolioSummary::PortfolioSummary(double buyingPower)
    : m_buyingPower{ buyingPower }
    , m_holdingBalance{ 0.0 }
    , m_borrowedBalance{ 0.0 }
    , m_totalPL{ 0.0 }
    , m_totalShares{ 0 }
{
}

PortfolioSummary::PortfolioSummary(double buyingPower, int totalShares)
    : m_buyingPower{ buyingPower }
    , m_holdingBalance{ 0.0 }
    , m_borrowedBalance{ 0.0 }
    , m_totalPL{ 0.0 }
    , m_totalShares{ totalShares }
{
}

PortfolioSummary::PortfolioSummary(double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares)
    : m_buyingPower{ buyingPower }
    , m_holdingBalance{ holdingBalance }
    , m_borrowedBalance{ borrowedBalance }
    , m_totalPL{ totalPL }
    , m_totalShares{ totalShares }
{
}

void PortfolioSummary::holdBalance(double value)
{
    m_buyingPower -= value; // deduct the pending transaction price from the buying power
    m_holdingBalance += value; // and update the total holding balance
}

void PortfolioSummary::releaseBalance(double value)
{
    m_buyingPower += value;
    m_holdingBalance -= value;
}

void PortfolioSummary::borrowBalance(double value)
{
    m_buyingPower -= value;
    m_borrowedBalance += value;
}

void PortfolioSummary::returnBalance(double value)
{
    m_buyingPower += value * 2; // multiplied by 2 to account for both borrowing and selling
    m_borrowedBalance -= value;
}

void PortfolioSummary::addBuyingPower(double value)
{
    m_buyingPower += value;
}

void PortfolioSummary::addTotalPL(double value)
{
    m_totalPL += value;
}

void PortfolioSummary::addTotalShares(int value)
{
    m_totalShares += value;
}

double PortfolioSummary::getBuyingPower() const
{
    return m_buyingPower;
}

double PortfolioSummary::getHoldingBalance() const
{
    return m_holdingBalance;
}

double PortfolioSummary::getBorrowedBalance() const
{
    return m_borrowedBalance;
}

double PortfolioSummary::getTotalPL() const
{
    return m_totalPL;
}

int PortfolioSummary::getTotalShares() const
{
    return m_totalShares;
}
