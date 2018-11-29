#pragma once

namespace shift {

class PortfolioSummary {

public:
    PortfolioSummary();

    PortfolioSummary(double totalBP, int totalShares, double totalRealizedPL);

    double getTotalBP() const;
    int getTotalShares() const;
    double getTotalRealizedPL() const;

    double getOpenBP() const;
    bool isOpenBPReady() const;

    void setTotalBP(double totalBP);
    void setTotalShares(int totalShares);
    void setTotalRealizedPL(double totalRealizedPL);

    void setOpenBP(double openBP);

private:
    double m_totalBP;
    int m_totalShares;
    double m_totalRealizedPL;

    double m_openBP;
    bool m_isOpenBPReady = false;
};

} // shift
