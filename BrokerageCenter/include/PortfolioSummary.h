#pragma once

class PortfolioSummary {
    double m_buyingPower;
    double m_holdingBalance;
    double m_borrowedBalance; /* When client attempt to sell the stock 
                               * but does not have any shares of this stock,
                               * client has to pay money for this stock,
                               * and the borrowed money will be returned to client 
                               * when client buy it later.
                               */
    double m_totalPL;
    int m_totalShares;

public:
    PortfolioSummary(double buyingPower);
    PortfolioSummary(double buyingPower, int totalShares);
    PortfolioSummary(double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares);

    void holdBalance(double value);
    void releaseBalance(double value);

    void borrowBalance(double value);
    void returnBalance(double value);

    void addBuyingPower(double value);
    void addTotalPL(double value);
    void addTotalShares(int value);

    double getBuyingPower() const;
    double getHoldingBalance() const;
    double getBorrowedBalance() const;
    double getTotalPL() const;
    int getTotalShares() const;
};
