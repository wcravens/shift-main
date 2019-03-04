#include "CandlestickDataPoint.h"

CandlestickDataPoint::CandlestickDataPoint() = default;

CandlestickDataPoint::CandlestickDataPoint(std::string symbol, double tempOpenPrice, double tempClosePrice, double tempHighPrice, double tempLowPrice, std::time_t tempTimeFrom)
    : m_symbol(std::move(symbol))
    , m_tempTimeFrom{ tempTimeFrom }
    , m_tempOpenPrice{ tempOpenPrice }
    , m_tempClosePrice{ tempClosePrice }
    , m_tempHighPrice{ tempHighPrice }
    , m_tempLowPrice{ tempLowPrice }
{
}

std::time_t CandlestickDataPoint::getTempTimeFrom() const
{
    return m_tempTimeFrom;
}

const std::string& CandlestickDataPoint::getSymbol() const
{
    return m_symbol;
}

double CandlestickDataPoint::getTempOpenPrice() const
{
    return m_tempOpenPrice;
}

double CandlestickDataPoint::getTempClosePrice() const
{
    return m_tempClosePrice;
}

double CandlestickDataPoint::getTempHighPrice() const
{
    return m_tempHighPrice;
}

double CandlestickDataPoint::getTempLowPrice() const
{
    return m_tempLowPrice;
}
