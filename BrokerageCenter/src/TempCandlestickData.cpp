#include "TempCandlestickData.h"

TempCandlestickData::TempCandlestickData() = default;

TempCandlestickData::TempCandlestickData(std::string symbol, double tempOpenPrice, double tempClosePrice, double tempHighPrice, double tempLowPrice, std::time_t tempTimeFrom)
    : m_symbol(std::move(symbol))
    , m_tempTimeFrom{ tempTimeFrom }
    , m_tempOpenPrice{ tempOpenPrice }
    , m_tempClosePrice{ tempClosePrice }
    , m_tempHighPrice{ tempHighPrice }
    , m_tempLowPrice{ tempLowPrice }
{
}

std::time_t TempCandlestickData::getTempTimeFrom() const
{
    return m_tempTimeFrom;
}

const std::string& TempCandlestickData::getSymbol() const
{
    return m_symbol;
}

double TempCandlestickData::getTempOpenPrice() const
{
    return m_tempOpenPrice;
}

double TempCandlestickData::getTempClosePrice() const
{
    return m_tempClosePrice;
}

double TempCandlestickData::getTempHighPrice() const
{
    return m_tempHighPrice;
}

double TempCandlestickData::getTempLowPrice() const
{
    return m_tempLowPrice;
}
