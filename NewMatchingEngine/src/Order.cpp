#include "Order.h"

/// Public ////////////////////////////////////////////////////////////////////

Order::Order(const Order& other)
    : m_isGlobal(other.getIsGlobal())
    , m_orderID(other.getOrderID())
    , m_brokerID(other.getBrokerID())
    , m_traderID(other.getTraderID())
    , m_destination(other.getDestination())
    , m_upInstrument(other.getInstrument()->clone())
    , m_side(other.getSide())
    , m_status(other.getStatus())
    , m_orderQuantity(other.getOrderQuantity())
    , m_leavesQuantity(other.getLeavesQuantity())
    , m_cumulativeQuantity(other.getCumulativeQuantity())
    , m_time(other.getTime())
{
}

Order& Order::operator=(const Order& other) &
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
    : m_isGlobal(isGlobal)
    , m_orderID(std::move(orderID))
    , m_brokerID(std::move(brokerID))
    , m_traderID(std::move(traderID))
    , m_destination(std::move(destination))
    , m_upInstrument(std::move(instrument))
    , m_side(side)
    , m_status(Order::STATUS::NEW)
    , m_orderQuantity(orderQuantity)
    , m_leavesQuantity(orderQuantity)
    , m_cumulativeQuantity(0)
    , m_time(std::move(time))
{
}

std::unique_ptr<Order> Order::clone() const
{
    return std::unique_ptr<Order>(cloneImpl());
}

bool Order::getIsGlobal() const
{
    return m_isGlobal;
}

const std::string& Order::getOrderID() const
{
    return m_orderID;
}

const std::string& Order::getBrokerID() const
{
    return m_brokerID;
}

const std::string& Order::getTraderID() const
{
    return m_traderID;
}

const std::string& Order::getDestination() const
{
    return m_destination;
}

const Instrument* Order::getInstrument() const
{
    return m_upInstrument.get();
}

Order::SIDE Order::getSide() const
{
    return m_side;
}

Order::STATUS Order::getStatus() const
{
    return m_status;
}

double Order::getOrderQuantity() const
{
    return m_orderQuantity;
}

double Order::getLeavesQuantity() const
{
    return m_leavesQuantity;
}

double Order::getCumulativeQuantity() const
{
    return m_cumulativeQuantity;
}

const boost::posix_time::ptime& Order::getTime() const
{
    return m_time;
}
