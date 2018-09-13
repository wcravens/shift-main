#include "LimitOrder.h"

/// Protected /////////////////////////////////////////////////////////////////

LimitOrder* LimitOrder::cloneImpl() const // override
{
    return new LimitOrder(*this);
}

/// Public ////////////////////////////////////////////////////////////////////

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price)
    : Order(isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
          std::move(destination), std::move(instrument), side, orderQuantity,
          std::move(time))
    , m_price(price)
    , m_displayQuantity(orderQuantity)
    , m_timeInForce(LimitOrder::TIME_IN_FORCE::DAY)
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, double displayQuantity)
    : Order(isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
          std::move(destination), std::move(instrument), side, orderQuantity,
          std::move(time))
    , m_price(price)
    , m_displayQuantity(displayQuantity)
    , m_timeInForce(LimitOrder::TIME_IN_FORCE::DAY)
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, LimitOrder::TIME_IN_FORCE timeInForce)
    : Order(isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
          std::move(destination), std::move(instrument), side, orderQuantity,
          std::move(time))
    , m_price(price)
    , m_displayQuantity(orderQuantity)
    , m_timeInForce(timeInForce)
{
}

LimitOrder::LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time, double price, double displayQuantity, LimitOrder::TIME_IN_FORCE timeInForce)
    : Order(isGlobal, std::move(orderID), std::move(brokerID), std::move(traderID),
          std::move(destination), std::move(instrument), side, orderQuantity,
          std::move(time))
    , m_price(price)
    , m_displayQuantity(displayQuantity)
    , m_timeInForce(timeInForce)
{
}

Order::TYPE LimitOrder::getType() const // override
{
    return Order::TYPE::LIMIT;
}

double LimitOrder::getPrice() const
{
    return m_price;
}

double LimitOrder::getDisplayQuantity() const
{
    return m_displayQuantity;
}

LimitOrder::TIME_IN_FORCE LimitOrder::getTimeInForce() const
{
    return m_timeInForce;
}
