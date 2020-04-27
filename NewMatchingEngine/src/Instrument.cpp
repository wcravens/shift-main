#include "Instrument.h"

/// Public ////////////////////////////////////////////////////////////////////

Instrument::Instrument(std::string symbol)
    : m_symbol(std::move(symbol))
{
}

auto Instrument::clone() const -> std::unique_ptr<Instrument>
{
    return std::unique_ptr<Instrument>(cloneImpl());
}

auto Instrument::getTypeString() const -> std::string
{
    return instrumentTypeToString(getType());
}

auto Instrument::getSymbol() const -> const std::string&
{
    return m_symbol;
}
