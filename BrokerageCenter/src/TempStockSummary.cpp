#include "TempStockSummary.h"

TempStockSummary::TempStockSummary() = default;

TempStockSummary::TempStockSummary(std::string symbol, double tempOpenPrice, double tempClosePrice, double tempHighPrice, double tempLowPrice, std::time_t tempTimeFrom)
    : m_symbol(std::move(symbol))
    , m_tempTimeFrom{ tempTimeFrom }
    , m_tempOpenPrice{ tempOpenPrice }
    , m_tempClosePrice{ tempClosePrice }
    , m_tempHighPrice{ tempHighPrice }
    , m_tempLowPrice{ tempLowPrice }
{
}

std::time_t TempStockSummary::getTempTimeFrom() const
{
    return m_tempTimeFrom;
}

const std::string& TempStockSummary::getSymbol() const
{
    return m_symbol;
}

double TempStockSummary::getTempOpenPrice() const
{
    return m_tempOpenPrice;
}

double TempStockSummary::getTempClosePrice() const
{
    return m_tempClosePrice;
}

double TempStockSummary::getTempHighPrice() const
{
    return m_tempHighPrice;
}

double TempStockSummary::getTempLowPrice() const
{
    return m_tempLowPrice;
}
