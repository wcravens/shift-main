#pragma once

#include <string>

/**
*   @brief The "Position" item of the user's portfolio.
*/

class PortfolioItem {
    std::string m_symbol;
    int m_longShares; // expect the price to go up
    int m_shortShares; // expect the price to go down
    double m_longPrice;
    double m_shortPrice;
    double m_pl;
    double m_borrowedBalance;

public:
    PortfolioItem();
    PortfolioItem(const std::string& symbol);
    PortfolioItem(const std::string& symbol, int shares, double price);

    void addLongShares(int value);
    void addShortShares(int value);
    void addLongPrice(double value, int quantity);
    void addShortPrice(double value, int quantity);
    void addPL(double value);
    void addBorrowedBalance(double value);

    void resetLongShares();
    void resetShortShares();
    void resetLongPrice();
    void resetShortPrice();
    void resetPL();
    void resetBorrowedBalance();

    const std::string& getSymbol() const;
    int getSummaryShares() const;
    int getLongShares() const;
    int getShortShares() const;
    double getSummaryPrice() const;
    double getLongPrice() const;
    double getShortPrice() const;
    double getPL() const;
    double getBorrowedBalance() const;
};
