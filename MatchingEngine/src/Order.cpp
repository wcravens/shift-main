#include "Order.h"

Order::Order(const std::string& symbol, double price, int size, Order::Type type, const std::string& destination, const FIX::UtcTimeStamp& simulationTime)
    : m_symbol(symbol)
    , m_traderID("TRTH")
    , m_orderID("TRTH")
    , m_price(price)
    , m_size(size)
    , m_type(type)
    , m_destination(destination)
    , m_simulationTime(simulationTime)
{
}

Order::Order(const std::string& symbol, const std::string& traderID, const std::string& orderID, double price, int size, Order::Type type, const FIX::UtcTimeStamp& simulationTime)
    : m_symbol(symbol)
    , m_traderID(traderID)
    , m_orderID(orderID)
    , m_price(price)
    , m_size(size)
    , m_type(type)
    , m_destination("SHIFT")
    , m_simulationTime(simulationTime)
{
}

bool Order::operator==(const Order& other)
{
    return m_symbol == other.m_symbol
        && m_traderID == other.m_traderID
        && m_orderID == other.m_orderID
        && m_price == other.m_price
        && m_size == other.m_size
        && m_type == other.m_type
        && m_destination == other.m_destination;
}

const std::string& Order::getSymbol() const
{
    return m_symbol;
}

const std::string& Order::getTraderID() const
{
    return m_traderID;
}

const std::string& Order::getOrderID() const
{
    return m_orderID;
}

long Order::getMilli() const
{
    return m_milli;
}

double Order::getPrice() const
{
    return m_price;
}

int Order::getSize() const
{
    return m_size;
}

Order::Type Order::getType() const
{
    return m_type;
}

const std::string& Order::getDestination() const
{
    return m_destination;
}

const FIX::UtcTimeStamp& Order::getTime() const
{
    return m_simulationTime;
}

void Order::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void Order::setMilli(long milli)
{
    m_milli = milli;
}

void Order::setPrice(double price)
{
    m_price = price;
}

void Order::setSize(int size)
{
    m_size = size;
}

void Order::setType(Order::Type type)
{
    m_type = type;
}

void Order::setDestination(const std::string& destination)
{
    m_destination = destination;
}
