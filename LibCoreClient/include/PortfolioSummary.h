#pragma once

namespace shift {

class PortfolioSummary {

public:
    PortfolioSummary() = default;

    PortfolioSummary(double totalBP, int totalShares, double totalPL, double totalRealizedPL)
        : m_totalBP(totalBP)
        , m_totalShares(totalShares)
        , m_totalPL(totalPL)
        , m_totalRealizedPL(totalRealizedPL)
    {
    }

    double getTotalBP() const;
    int getTotalShares() const;
    double getTotalPL() const;
    double getTotalRealizedPL() const;

    double getOpenBP() const;
    bool isOpenBPReady() const;

    void setTotalBP(double totalBP);
    void setTotalShares(int totalShares);
    void setTotalPL(double totalPL);
    void setTotalRealizedPL(double totalRealizedPL);

    void setOpenBP(double openBP);

private:
    double m_totalBP;
    int m_totalShares;
    double m_totalPL;
    double m_totalRealizedPL;

    double m_openBP;
    bool m_isOpenBPReady = false;
};

} // shift
