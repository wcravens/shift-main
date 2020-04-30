#pragma once

#include <chrono>

namespace shift {

class PortfolioSummary {
public:
    PortfolioSummary();
    PortfolioSummary(double totalBP, int totalShares, double totalRealizedPL);

    auto isOpenBPReady() const -> bool;
    auto getOpenBP() const -> double;
    auto getTotalBP() const -> double;
    auto getTotalShares() const -> int;
    auto getTotalRealizedPL() const -> double;
    auto getTimestamp() const -> const std::chrono::system_clock::time_point&;

    void setOpenBP(double openBP);
    void setTotalBP(double totalBP);
    void setTotalShares(int totalShares);
    void setTotalRealizedPL(double totalRealizedPL);
    void setTimestamp();

private:
    bool m_isOpenBPReady = false;
    double m_openBP;
    double m_totalBP;
    int m_totalShares;
    double m_totalRealizedPL;
    std::chrono::system_clock::time_point m_timestamp;
};

} // shift
