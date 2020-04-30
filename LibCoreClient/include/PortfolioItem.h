
#pragma once

#include <chrono>
#include <string>

namespace shift {

class PortfolioItem {
public:
    PortfolioItem();
    PortfolioItem(std::string symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL);

    auto getSymbol() const -> const std::string&;
    auto getShares() const -> int;
    auto getLongShares() const -> int;
    auto getShortShares() const -> int;
    auto getPrice() const -> double;
    auto getLongPrice() const -> double;
    auto getShortPrice() const -> double;
    auto getRealizedPL() const -> double;
    auto getTimestamp() const -> const std::chrono::system_clock::time_point&;

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
