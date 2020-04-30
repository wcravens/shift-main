#include "CandlestickDataPoint.h"

CandlestickDataPoint::CandlestickDataPoint()
    : CandlestickDataPoint { "", 0.0, 0.0, 0.0, 0.0, std::time_t {} }
{
}

CandlestickDataPoint::CandlestickDataPoint(std::string symbol, double openPrice, double closePrice, double highPrice, double lowPrice, std::time_t timeFrom)
    : m_symbol { std::move(symbol) }
    , m_openPrice { openPrice }
    , m_closePrice { closePrice }
    , m_highPrice { highPrice }
    , m_lowPrice { lowPrice }
    , m_timeFrom { timeFrom }
{
}

auto CandlestickDataPoint::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto CandlestickDataPoint::getOpenPrice() const -> double
{
    return m_openPrice;
}

auto CandlestickDataPoint::getClosePrice() const -> double
{
    return m_closePrice;
}

auto CandlestickDataPoint::getHighPrice() const -> double
{
    return m_highPrice;
}

auto CandlestickDataPoint::getLowPrice() const -> double
{
    return m_lowPrice;
}

auto CandlestickDataPoint::getTimeFrom() const -> std::time_t
{
    return m_timeFrom;
}
