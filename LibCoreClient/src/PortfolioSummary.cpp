#include "PortfolioSummary.h"

double shift::PortfolioSummary::getTotalBP() const
{
    return m_totalBP;
}

int shift::PortfolioSummary::getTotalShares() const
{
    return m_totalShares;
}

double shift::PortfolioSummary::getTotalPL() const
{
    return m_totalPL;
}

double shift::PortfolioSummary::getTotalRealizedPL() const
{
    return m_totalRealizedPL;
}

double shift::PortfolioSummary::getOpenBP() const
{
    return m_openBP;
}

bool shift::PortfolioSummary::isOpenBPReady() const
{
    return m_isOpenBPReady;
}

void shift::PortfolioSummary::setTotalBP(double totalBP)
{
    m_totalBP = totalBP;
    m_isOpenBPReady = true;
}

void shift::PortfolioSummary::setTotalShares(int totalShares)
{
    m_totalShares = totalShares;
}

void shift::PortfolioSummary::setTotalPL(double totalPL)
{
    m_totalPL = totalPL;
}

void shift::PortfolioSummary::setTotalRealizedPL(double totalRealizedPL)
{
    m_totalRealizedPL = totalRealizedPL;
}

void shift::PortfolioSummary::setOpenBP(double openBP)
{
    m_openBP = openBP;
}
