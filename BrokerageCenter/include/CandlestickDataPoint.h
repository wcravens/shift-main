#pragma once

#include <ctime>
#include <string>

class CandlestickDataPoint {
    std::string m_symbol;
    std::time_t m_tempTimeFrom;
    double m_tempOpenPrice;
    double m_tempClosePrice;
    double m_tempHighPrice;
    double m_tempLowPrice;

public:
    CandlestickDataPoint();
    CandlestickDataPoint(std::string symbol, double tempOpenPrice, double tempClosePrice, double tempHighPrice, double tempLowPrice, std::time_t tempTimeFrom);

    std::time_t getTempTimeFrom() const;
    const std::string& getSymbol() const;
    double getTempOpenPrice() const;
    double getTempClosePrice() const;
    double getTempHighPrice() const;
    double getTempLowPrice() const;
};
