#include "PortfolioSummary.h"

shift::PortfolioSummary::PortfolioSummary()
    : m_isOpenBPReady(false)
    , m_openBP(0.0)
    , m_totalBP(0.0)
    , m_totalShares(0.0)
    , m_totalRealizedPL(0.0)
    , m_timestamp(std::chrono::system_clock::now())
{
}

shift::PortfolioSummary::PortfolioSummary(double totalBP, int totalShares, double totalRealizedPL)
    : m_isOpenBPReady(true)
    , m_openBP(totalBP)
    , m_totalBP(totalBP)
    , m_totalShares(totalShares)
    , m_totalRealizedPL(totalRealizedPL)
    , m_timestamp(std::chrono::system_clock::now())
{
}

bool shift::PortfolioSummary::isOpenBPReady() const
{
    return m_isOpenBPReady;
}

double shift::PortfolioSummary::getOpenBP() const
{
    return m_openBP;
}

double shift::PortfolioSummary::getTotalBP() const
{
    return m_totalBP;
}

int shift::PortfolioSummary::getTotalShares() const
{
    return m_totalShares;
}

double shift::PortfolioSummary::getTotalRealizedPL() const
{
    return m_totalRealizedPL;
}

const std::chrono::system_clock::time_point& shift::PortfolioSummary::getTimestamp() const
{
    return m_timestamp;
}

void shift::PortfolioSummary::setOpenBP(double openBP)
{
    m_openBP = openBP;
    m_isOpenBPReady = true;
}

void shift::PortfolioSummary::setTotalBP(double totalBP)
{
    m_totalBP = totalBP;
}

void shift::PortfolioSummary::setTotalShares(int totalShares)
{
    m_totalShares = totalShares;
}

void shift::PortfolioSummary::setTotalRealizedPL(double totalRealizedPL)
{
    m_totalRealizedPL = totalRealizedPL;
}

void shift::PortfolioSummary::setTimestamp()
{
    m_timestamp = std::chrono::system_clock::now();
}
