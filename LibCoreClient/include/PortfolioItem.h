
#pragma once

#include <chrono>
#include <string>

namespace shift {

class PortfolioItem {

public:
    PortfolioItem() = default; // required for map

    PortfolioItem(const std::string& symbol, int shares, double price, double realizedPL);

    const std::string& getSymbol() const;
    int getShares() const;
    double getPrice() const;
    double getRealizedPL() const;
    const std::chrono::system_clock::time_point& getTimestamp() const;

private:
    std::string m_symbol;
    int m_shares;
    double m_price;
    double m_realizedPL;
    std::chrono::system_clock::time_point m_timestamp;
};

} // shift
