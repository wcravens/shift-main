#include "Instrument.h"

/// Public ////////////////////////////////////////////////////////////////////

Instrument::Instrument(std::string symbol)
    : m_symbol(std::move(symbol))
{
}

std::unique_ptr<Instrument> Instrument::clone() const
{
    return std::unique_ptr<Instrument>(cloneImpl());
}

std::string Instrument::getTypeString() const
{
    return instrumentTypeToString(getType());
}

const std::string& Instrument::getSymbol() const
{
    return m_symbol;
}
