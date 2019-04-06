#include "Quote.h"

Quote::Quote(const std::string& symbol, const std::string& traderID, const std::string& orderID, double price, int size, char orderType, const FIX::UtcTimeStamp& time)
    : m_symbol(symbol)
    , m_traderID(traderID)
    , m_orderID(orderID)
    , m_price(price)
    , m_size(size)
    , m_orderType(orderType)
    , m_destination("SHIFT")
    , m_time(time)
{
}

Quote::Quote(const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time)
    : m_symbol(symbol)
    , m_traderID("TR")
    , m_orderID("Thomson")
    , m_price(price)
    , m_size(size)
    , m_orderType('1')
    , m_destination(destination)
    , m_time(time)
{
}

const std::string& Quote::getSymbol() const
{
    return m_symbol;
}

const std::string& Quote::getTraderID() const
{
    return m_traderID;
}

const std::string& Quote::getOrderID() const
{
    return m_orderID;
}

long Quote::getMilli() const
{
    return m_milli;
}

double Quote::getPrice() const
{
    return m_price;
}

int Quote::getSize() const
{
    return m_size;
}

char Quote::getOrderType() const
{
    return m_orderType;
}

const std::string& Quote::getDestination() const
{
    return m_destination;
}

const FIX::UtcTimeStamp& Quote::getTime() const
{
    return m_time;
}

void Quote::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void Quote::setMilli(long milli)
{
    m_milli = milli;
}

void Quote::setPrice(double price)
{
    m_price = price;
}

void Quote::setSize(int size)
{
    m_size = size;
}

void Quote::setOrderType(char orderType)
{
    m_orderType = orderType;
}

void Quote::setDestination(const std::string& destination)
{
    m_destination = destination;
}
