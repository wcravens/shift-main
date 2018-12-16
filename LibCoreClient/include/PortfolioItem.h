
#pragma once

#include <chrono>
#include <string>

namespace shift {

class PortfolioItem {

public:
    PortfolioItem() = default; // required for map

    PortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL);

    const std::string& getSymbol() const;
    int getShares() const;
    int getLongShares() const;
    int getShortShares() const;
    double getPrice() const;
    double getLongPrice() const;
    double getShortPrice() const;
    double getRealizedPL() const;
    const std::chrono::system_clock::time_point& getTimestamp() const;

private:
    std::string m_symbol;
    int m_longShares;
    int m_shortShares;
    double m_longPrice;
    double m_shortPrice;
    double m_realizedPL;
    std::chrono::system_clock::time_point m_timestamp;
};

} // shift
