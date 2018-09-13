
#pragma once

#include <string>

namespace shift {

class PortfolioItem {

public:
    PortfolioItem() = default;

    PortfolioItem(std::string symbol, int shares, double price, double pl)
        : m_symbol(symbol)
        , m_shares(shares)
        , m_price(price)
        , m_pl(pl)
    {
    }

    const std::string& getSymbol() const;
    int getShares() const;
    double getPrice() const;
    double getPL() const;

private:
    std::string m_symbol;
    int m_shares;
    double m_price;
    double m_pl;
};

} // shift
