#include "MarketOrder.h"

/// Protected /////////////////////////////////////////////////////////////////

/* virtual */ auto MarketOrder::cloneImpl() const -> MarketOrder* // override
{
    return new MarketOrder(*this);
}

/// Public ////////////////////////////////////////////////////////////////////

MarketOrder::MarketOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time)
    : Order(isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
        std::move(destination), std::move(instrument), side, orderQuantity,
        std::move(time))
{
}

/* virtual */ auto MarketOrder::getType() const -> Order::TYPE // override
{
    return Order::TYPE::MARKET;
}
