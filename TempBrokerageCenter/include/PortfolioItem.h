#pragma once

#include <string>

/**
*   @brief The "Positions" item of the user's portfolio.
*/

class PortfolioItem {
    std::string m_symbol;
    double m_borrowedBalance;
    double m_pl;
    double m_longPrice;
    double m_shortPrice;
    int m_longShares; // expect the price to go up
    int m_shortShares; // expect the price to go down

public:
    PortfolioItem();
    PortfolioItem(const std::string& symbol);
    PortfolioItem(const std::string& symbol, int shares, double price);
    PortfolioItem(const std::string& symbol, double borrowedBalace, double pl, double longPrice, double shortPrice, int longShares, int shortShares); //> For parametric use, i.e. to explicitly configurate the initial portfolio item.

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
