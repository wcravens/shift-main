#include "LimitOrder.h"

/// Protected /////////////////////////////////////////////////////////////////

/* virtual*/ auto LimitOrder::cloneImpl() const -> LimitOrder* // override
{
    return new LimitOrder(*this);
}

/// Public ////////////////////////////////////////////////////////////////////

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price)
    : Order { isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
        std::move(destination), std::move(instrument), side, orderQuantity,
        std::move(time) }
    , m_price { price }
    , m_displayQuantity { orderQuantity }
    , m_timeInForce { LimitOrder::TIME_IN_FORCE::DAY }
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, double displayQuantity)
    : Order { isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
        std::move(destination), std::move(instrument), side, orderQuantity,
        std::move(time) }
    , m_price { price }
    , m_displayQuantity { displayQuantity }
    , m_timeInForce { LimitOrder::TIME_IN_FORCE::DAY }
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, LimitOrder::TIME_IN_FORCE timeInForce)
    : Order { isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
        std::move(destination), std::move(instrument), side, orderQuantity,
        std::move(time) }
    , m_price { price }
    , m_displayQuantity { orderQuantity }
    , m_timeInForce { timeInForce }
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, double displayQuantity, LimitOrder::TIME_IN_FORCE timeInForce)
    : Order { isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
        std::move(destination), std::move(instrument), side, orderQuantity,
        std::move(time) }
    , m_price { price }
    , m_displayQuantity { displayQuantity }
    , m_timeInForce { timeInForce }
{
}

/* virtual */ auto LimitOrder::getType() const -> Order::TYPE // override
{
    return Order::TYPE::LIMIT;
}

auto LimitOrder::getPrice() const -> double
{
    return m_price;
}

auto LimitOrder::getDisplayQuantity() const -> double
{
    return m_displayQuantity;
}

auto LimitOrder::getTimeInForce() const -> LimitOrder::TIME_IN_FORCE
{
    return m_timeInForce;
}
