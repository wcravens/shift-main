#pragma once

#include <string>

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
    PortfolioItem(std::string symbol);
    PortfolioItem(std::string symbol, double price, int shares);

    void addBorrowedBalance(double value);
    void addPL(double value);
    void addLongPrice(double value, int quantity);
    void addShortPrice(double value, int quantity);
    void addLongShares(int value);
    void addShortShares(int value);

    void resetBorrowedBalance();
    void resetPL();
    void resetLongPrice();
    void resetShortPrice();
    void resetLongShares();
    void resetShortShares();

    const std::string& getSymbol() const;
    double getBorrowedBalance() const;
    double getPL() const;
    double getSummaryPrice() const;
    double getLongPrice() const;
    double getShortPrice() const;
    int getSummaryShares() const;
    int getLongShares() const;
    int getShortShares() const;
};
