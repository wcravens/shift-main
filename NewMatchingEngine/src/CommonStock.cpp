#include "CommonStock.h"

/// Protected /////////////////////////////////////////////////////////////////

CommonStock* CommonStock::cloneImpl() const // override
{
    return new CommonStock(*this);
}

/// Public ////////////////////////////////////////////////////////////////////

CommonStock::CommonStock(std::string symbol)
    : Instrument(std::move(symbol))
{
}

Instrument::TYPE CommonStock::getType() const // override
{
    return Instrument::TYPE::COMMON_STOCK;
}
