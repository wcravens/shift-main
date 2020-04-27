#include "CommonStock.h"

/// Protected /////////////////////////////////////////////////////////////////

/* virtual */ auto CommonStock::cloneImpl() const -> CommonStock* // override
{
    return new CommonStock(*this);
}

/// Public ////////////////////////////////////////////////////////////////////

CommonStock::CommonStock(std::string symbol)
    : Instrument(std::move(symbol))
{
}

/* virtual */ auto CommonStock::getType() const -> Instrument::TYPE // override
{
    return Instrument::TYPE::COMMON_STOCK;
}
