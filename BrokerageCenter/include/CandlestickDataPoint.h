#pragma once

#include <ctime>
#include <string>

class CandlestickDataPoint {
    std::string m_symbol;
    std::time_t m_timeFrom;
    double m_openPrice;
    double m_closePrice;
    double m_highPrice;
    double m_lowPrice;

public:
    CandlestickDataPoint();
    CandlestickDataPoint(std::string symbol, double openPrice, double closePrice, double highPrice, double lowPrice, std::time_t timeFrom);

    std::time_t getTimeFrom() const;
    const std::string& getSymbol() const;
    double getOpenPrice() const;
    double getClosePrice() const;
    double getHighPrice() const;
    double getLowPrice() const;
};
