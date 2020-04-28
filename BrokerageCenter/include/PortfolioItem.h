#pragma once

#include <string>

/**
 * @brief The "Positions" item of the user's portfolio.
 */
class PortfolioItem {
public:
    PortfolioItem() = default;
    PortfolioItem(std::string symbol);
    PortfolioItem(std::string symbol, int shares, double price);
    PortfolioItem(std::string symbol, double borrowedBalace, double pl, double longPrice, double shortPrice, int longShares, int shortShares); //> For parametric use, i.e. to explicitly configurate the initial portfolio item.

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

    auto getSymbol() const -> const std::string&;
    auto getSummaryShares() const -> int;
    auto getLongShares() const -> int;
    auto getShortShares() const -> int;
    auto getSummaryPrice() const -> double;
    auto getLongPrice() const -> double;
    auto getShortPrice() const -> double;
    auto getPL() const -> double;
    auto getBorrowedBalance() const -> double;

private:
    std::string m_symbol;
    double m_borrowedBalance;
    double m_pl;
    double m_longPrice;
    double m_shortPrice;
    int m_longShares; // expect the price to go up
    int m_shortShares; // expect the price to go down
};
