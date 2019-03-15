#include "Order.h"

Order::Order(Order::Type type, const std::string& symbol, int size, double price, const std::string& id, const std::string& username)
    : m_type(type)
    , m_symbol(symbol)
    , m_size(size)
    , m_price(price)
    , m_id(id)
    , m_username(username)
    , m_status(Order::Status::PENDING_NEW)
{
}

Order::Type Order::getType() const
{
    return m_type;
}

const std::string& Order::getSymbol() const
{
    return m_symbol;
}

int Order::getSize() const
{
    return m_size;
}

double Order::getPrice() const
{
    return m_price;
}

const std::string& Order::getID() const
{
    return m_id;
}

const std::string& Order::getUsername() const
{
    return m_username;
}

Order::Status Order::getStatus() const
{
    return m_status;
}

void Order::setType(Order::Type type)
{
    m_type = type;
}

void Order::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void Order::setSize(int size)
{
    m_size = size;
}

void Order::setPrice(double price)
{
    m_price = price;
}

void Order::setID(const std::string& id)
{
    m_id = id;
}

void Order::setUsername(const std::string& username)
{
    m_username = username;
}

void Order::setStatus(Order::Status status)
{
    m_status = status;
}
