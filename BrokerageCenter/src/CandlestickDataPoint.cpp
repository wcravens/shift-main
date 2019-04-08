#include "CandlestickDataPoint.h"

CandlestickDataPoint::CandlestickDataPoint(const std::string& symbol, double openPrice, double closePrice, double highPrice, double lowPrice, std::time_t timeFrom)
    : m_symbol(symbol)
    , m_openPrice(openPrice)
    , m_closePrice(closePrice)
    , m_highPrice(highPrice)
    , m_lowPrice(lowPrice)
    , m_timeFrom(timeFrom)
{
}

const std::string& CandlestickDataPoint::getSymbol() const
{
    return m_symbol;
}

double CandlestickDataPoint::getOpenPrice() const
{
    return m_openPrice;
}

double CandlestickDataPoint::getClosePrice() const
{
    return m_closePrice;
}

double CandlestickDataPoint::getHighPrice() const
{
    return m_highPrice;
}

double CandlestickDataPoint::getLowPrice() const
{
    return m_lowPrice;
}

std::time_t CandlestickDataPoint::getTimeFrom() const
{
    return m_timeFrom;
}
