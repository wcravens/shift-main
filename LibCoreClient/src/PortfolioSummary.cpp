#include "PortfolioSummary.h"

namespace shift {

PortfolioSummary::PortfolioSummary()
    : m_isOpenBPReady { false }
    , m_openBP { 0.0 }
    , m_totalBP { 0.0 }
    , m_totalShares { 0 }
    , m_totalRealizedPL { 0.0 }
    , m_timestamp { std::chrono::system_clock::time_point() }
{
}

PortfolioSummary::PortfolioSummary(double totalBP, int totalShares, double totalRealizedPL)
    : m_isOpenBPReady { true }
    , m_openBP { totalBP }
    , m_totalBP { totalBP }
    , m_totalShares { totalShares }
    , m_totalRealizedPL { totalRealizedPL }
    , m_timestamp { std::chrono::system_clock::now() }
{
}

auto PortfolioSummary::isOpenBPReady() const -> bool
{
    return m_isOpenBPReady;
}

auto PortfolioSummary::getOpenBP() const -> double
{
    return m_openBP;
}

auto PortfolioSummary::getTotalBP() const -> double
{
    return m_totalBP;
}

auto PortfolioSummary::getTotalShares() const -> int
{
    return m_totalShares;
}

auto PortfolioSummary::getTotalRealizedPL() const -> double
{
    return m_totalRealizedPL;
}

auto PortfolioSummary::getTimestamp() const -> const std::chrono::system_clock::time_point&
{
    return m_timestamp;
}

void PortfolioSummary::setOpenBP(double openBP)
{
    m_openBP = openBP;
    m_isOpenBPReady = true;
}

void PortfolioSummary::setTotalBP(double totalBP)
{
    m_totalBP = totalBP;
}

void PortfolioSummary::setTotalShares(int totalShares)
{
    m_totalShares = totalShares;
}

void PortfolioSummary::setTotalRealizedPL(double totalRealizedPL)
{
    m_totalRealizedPL = totalRealizedPL;
}

void PortfolioSummary::setTimestamp()
{
    m_timestamp = std::chrono::system_clock::now();
}

} // shift
