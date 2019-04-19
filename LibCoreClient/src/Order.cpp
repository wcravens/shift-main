#include "Order.h"

#include <cmath>

/* static */ inline double shift::Order::s_decimalTruncate(double value, int precision)
{
    return std::trunc(value * std::pow(10.0, precision)) / std::pow(10.0, precision);
}

shift::Order::Order(shift::Order::Type type, const std::string& symbol, int size, double price, const std::string& id)
    : m_type(type)
    , m_symbol(symbol)
    , m_size(size)
    , m_executedSize(0)
    , m_price(price)
    , m_executedPrice(0.0)
    , m_id(id)
    , m_status(shift::Order::Status::PENDING_NEW)
    , m_timestamp(std::chrono::system_clock::now())
{
    if (m_price <= 0.0) {
        m_price = 0.0;
        if (m_type == shift::Order::Type::LIMIT_BUY) {
            m_type = shift::Order::Type::MARKET_BUY;
        } else if (m_type == shift::Order::Type::LIMIT_SELL) {
            m_type = shift::Order::Type::MARKET_SELL;
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

std::string shift::Order::getTypeString() const
{
    switch (m_type) {
    case LIMIT_BUY: {
        return "Limit Buy";
    }
    case LIMIT_SELL: {
        return "Limit Sell";
    }
    case MARKET_BUY: {
        return "Market Buy";
    }
    case MARKET_SELL: {
        return "Market Sell";
    }
    case CANCEL_BID: {
        return "Cancel Bid";
    }
    case CANCEL_ASK: {
        return "Cancel Ask";
    }
    }
    return "N/A";
}

const std::string& shift::Order::getSymbol() const
{
    return m_symbol;
}

int shift::Order::getSize() const
{
    return m_size;
}

int shift::Order::getExecutedSize() const
{
    return m_executedSize;
}

double shift::Order::getPrice() const
{
    return m_price;
}

double shift::Order::getExecutedPrice() const
{
    return m_executedPrice;
}

const std::string& shift::Order::getID() const
{
    return m_id;
}

shift::Order::Status shift::Order::getStatus() const
{
    return m_status;
}

std::string shift::Order::getStatusString() const
{
    switch (m_status) {
    case PENDING_NEW: {
        return "Pending New";
    }
    case NEW: {
        return "New";
    }
    case PARTIALLY_FILLED: {
        return "Partially Filled";
    }
    case FILLED: {
        return "Filled";
    }
    case CANCELED: {
        return "Canceled";
    }
    case PENDING_CANCEL: {
        return "Pending Cancel";
    }
    case REJECTED: {
        return "Rejected";
    }
    }
    return "N/A";
}

const std::chrono::system_clock::time_point& shift::Order::getTimestamp() const
{
    return m_timestamp;
}

void shift::Order::setType(shift::Order::Type type)
{
    m_type = type;
}

void shift::Order::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
}

void shift::Order::setSize(int size)
{
    m_size = size;
}

void shift::Order::setExecutedSize(int executedSize)
{
    m_executedSize = executedSize;
}

void shift::Order::setPrice(double price)
{
    m_price = s_decimalTruncate(price, 2);
}

void shift::Order::setExecutedPrice(double executedPrice)
{
    m_executedPrice = executedPrice;
}

void shift::Order::setID(const std::string& id)
{
    m_id = id;
}

void shift::Order::setStatus(shift::Order::Status status)
{
    m_status = status;
}

void shift::Order::setTimestamp()
{
    m_timestamp = std::chrono::system_clock::now();
}
