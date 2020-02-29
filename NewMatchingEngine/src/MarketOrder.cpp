#include "MarketOrder.h"

/// Protected /////////////////////////////////////////////////////////////////

MarketOrder* MarketOrder::cloneImpl() const // override
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

Order::TYPE MarketOrder::getType() const // override
{
    return Order::TYPE::MARKET;
}
