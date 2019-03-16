#pragma once

#include <ctime>
#include <string>

class CandlestickDataPoint {
public:
    CandlestickDataPoint() = default;
    CandlestickDataPoint(const std::string& symbol, double openPrice, double closePrice, double highPrice, double lowPrice, std::time_t timeFrom);

    const std::string& getSymbol() const;
    double getOpenPrice() const;
    double getClosePrice() const;
    double getHighPrice() const;
    double getLowPrice() const;
    std::time_t getTimeFrom() const;

private:
    std::string m_symbol;
    double m_openPrice;
    double m_closePrice;
    double m_highPrice;
    double m_lowPrice;
    std::time_t m_timeFrom;
};
