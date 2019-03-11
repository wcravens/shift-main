#include "Order.h"

#include <cmath>


inline double shift::Order::s_decimalTruncate(double value, int precision)
{
    return std::trunc(value * std::pow(10.0, precision)) / std::pow(10.0, precision);
}

shift::Order::Order()
{
}

shift::Order::Order(shift::Order::Type type, const std::string& symbol, int size, double price, const std::string& id)
    : m_type(type)
    , m_symbol(symbol)
    , m_size(size)
    , m_price(price)
    , m_id(id)
    , m_timestamp(std::chrono::system_clock::now())
    , m_status(Status::PENDING_NEW)
{
    if (m_type == Type::CANCEL_BID || m_type == Type::CANCEL_ASK) {
        m_status = Status::PENDING_CANCEL;
    }

    if (m_price <= 0.0) {
        m_price = 0.0;
        if (m_type == Type::LIMIT_BUY) {
            m_type = Type::MARKET_BUY;
        } else if (m_type == Type::LIMIT_SELL) {
            m_type = Type::MARKET_SELL;
        }
    } else {
        m_price = s_decimalTruncate(m_price, 2);
    }

    if (m_id.empty()) {
        m_id = shift::crossguid::newGuid().str();
    }
}

shift::Order::Type shift::Order::getType() const
{
    return m_type;
}

const std::string& shift::Order::getSymbol() const
{
    return m_symbol;
}

int shift::Order::getSize() const
{
    return m_size;
}

double shift::Order::getPrice() const
{
    return m_price;
}

const std::string& shift::Order::getID() const
{
    return m_id;
}

const std::chrono::system_clock::time_point& shift::Order::getTimestamp() const
{
    return m_timestamp;
}

shift::Order::Status shift::Order::getStatus() const
{
    return m_status;
}

void shift::Order::setType(Type type)
{
    m_type = type;

    if (m_type == Type::CANCEL_BID || m_type == Type::CANCEL_ASK) {
        m_status = Status::PENDING_CANCEL;
    }
}

void shift::Order::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void shift::Order::setSize(int size)
{
    m_size = size;
}

void shift::Order::setPrice(double price)
{
    m_price = s_decimalTruncate(price, 2);
}

void shift::Order::setID(const std::string& id)
{
    m_id = id;
}

void shift::Order::setTimestamp()
{
    m_timestamp = std::chrono::system_clock::now();
}

void shift::Order::setStatus(Status status)
{
    m_status = status;

    if (m_status == Status::PENDING_CANCEL) {
        if (m_type == Type::LIMIT_BUY) {
            m_type = Type::CANCEL_BID;
        } else if (m_type == Type::LIMIT_SELL) {
            m_type = Type::CANCEL_ASK;
        }
    }
}
