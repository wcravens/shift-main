
#pragma once

#include <string>

namespace shift {

class PortfolioItem {

public:
    PortfolioItem() = default; // required for map

    PortfolioItem(const std::string& symbol);

    PortfolioItem(const std::string& symbol, int shares, double price, double realizedPL);

    const std::string& getSymbol() const;
    int getShares() const;
    double getPrice() const;
    double getRealizedPL() const;

private:
    std::string m_symbol;
    int m_shares;
    double m_price;
    double m_realizedPL;
};

} // shift
