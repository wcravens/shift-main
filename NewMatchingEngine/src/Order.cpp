#include "Order.h"

/// Public ////////////////////////////////////////////////////////////////////

Order::Order(const Order& other)
    : m_isGlobal { other.getIsGlobal() }
    , m_orderID { other.getOrderID() }
    , m_brokerID { other.getBrokerID() }
    , m_traderID { other.getTraderID() }
    , m_destination { other.getDestination() }
    , m_upInstrument { other.getInstrument()->clone() }
    , m_side { other.getSide() }
    , m_status { other.getStatus() }
    , m_orderQuantity { other.getOrderQuantity() }
    , m_leavesQuantity { other.getLeavesQuantity() }
    , m_cumulativeQuantity { other.getCumulativeQuantity() }
    , m_time { other.getTime() }
{
}

auto Order::operator=(const Order& other) & -> Order&
{
    m_isGlobal = other.getIsGlobal();
    m_orderID = other.getOrderID();
    m_brokerID = other.getBrokerID();
    m_traderID = other.getTraderID();
    m_destination = other.getDestination();
    m_upInstrument = other.getInstrument()->clone();
    m_side = other.getSide();
    m_status = other.getStatus();
    m_orderQuantity = other.getOrderQuantity();
    m_leavesQuantity = other.getLeavesQuantity();
    m_cumulativeQuantity = other.getCumulativeQuantity();
    m_time = other.getTime();

    return *this;
}

Order::Order(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
    std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
    boost::posix_time::ptime time)
    : m_isGlobal { isGlobal }
    , m_orderID { std::move(orderID) }
    , m_brokerID { std::move(brokerID) }
    , m_traderID { std::move(traderID) }
    , m_destination { std::move(destination) }
    , m_upInstrument { std::move(instrument) }
    , m_side { side }
    , m_status { Order::STATUS::NEW }
    , m_orderQuantity { orderQuantity }
    , m_leavesQuantity { orderQuantity }
    , m_cumulativeQuantity { 0 }
    , m_time { std::move(time) }
{
}

auto Order::clone() const -> std::unique_ptr<Order>
{
    return std::unique_ptr<Order>(cloneImpl());
}

auto Order::getIsGlobal() const -> bool
{
    return m_isGlobal;
}

auto Order::getOrderID() const -> const std::string&
{
    return m_orderID;
}

auto Order::getBrokerID() const -> const std::string&
{
    return m_brokerID;
}

auto Order::getTraderID() const -> const std::string&
{
    return m_traderID;
}

auto Order::getDestination() const -> const std::string&
{
    return m_destination;
}

auto Order::getInstrument() const -> const Instrument*
{
    return m_upInstrument.get();
}

auto Order::getSide() const -> Order::SIDE
{
    return m_side;
}

auto Order::getStatus() const -> Order::STATUS
{
    return m_status;
}

auto Order::getOrderQuantity() const -> double
{
    return m_orderQuantity;
}

auto Order::getLeavesQuantity() const -> double
{
    return m_leavesQuantity;
}

auto Order::getCumulativeQuantity() const -> double
{
    return m_cumulativeQuantity;
}

auto Order::getTime() const -> const boost::posix_time::ptime&
{
    return m_time;
}
