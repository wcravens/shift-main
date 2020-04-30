#pragma once

#include <ctime>
#include <string>

class CandlestickDataPoint {
public:
    CandlestickDataPoint();
    CandlestickDataPoint(std::string symbol, double openPrice, double closePrice, double highPrice, double lowPrice, std::time_t timeFrom);

    auto getSymbol() const -> const std::string&;
    auto getOpenPrice() const -> double;
    auto getClosePrice() const -> double;
    auto getHighPrice() const -> double;
    auto getLowPrice() const -> double;
    auto getTimeFrom() const -> std::time_t;

private:
    std::string m_symbol;
    double m_openPrice;
    double m_closePrice;
    double m_highPrice;
    double m_lowPrice;
    std::time_t m_timeFrom;
};
