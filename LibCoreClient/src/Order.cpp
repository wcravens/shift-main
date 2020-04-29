#include "Order.h"

#include <cmath>

namespace shift {

/* static */ inline auto Order::s_roundNearest(double value, double nearest) -> double
{
    return std::round(value / nearest) * nearest;
}

Order::Order(Order::Type type, std::string symbol, int size, double price /* = 0.0 */, std::string id /* = "" */)
    : m_type { type }
    , m_symbol { std::move(symbol) }
    , m_size { size }
    , m_executedSize { 0 }
    , m_price { price }
    , m_executedPrice { 0.0 }
    , m_id { std::move(id) }
    , m_status { Order::Status::PENDING_NEW }
    , m_timestamp { std::chrono::system_clock::now() }
{
    if (m_price <= 0.0) {
        m_price = 0.0;
        if (m_type == Order::Type::LIMIT_BUY) {
            m_type = Order::Type::MARKET_BUY;
        } else if (m_type == Order::Type::LIMIT_SELL) {
            m_type = Order::Type::MARKET_SELL;
        }
    } else {
        m_price = s_roundNearest(m_price, 0.01);
    }

    if (m_id.empty()) {
        m_id = crossguid::newGuid().str();
    }
}

auto Order::getType() const -> Order::Type
{
    return m_type;
}

auto Order::getTypeString() const -> std::string
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

auto Order::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto Order::getSize() const -> int
{
    return m_size;
}

auto Order::getExecutedSize() const -> int
{
    return m_executedSize;
}

auto Order::getPrice() const -> double
{
    return m_price;
}

auto Order::getExecutedPrice() const -> double
{
    return m_executedPrice;
}

auto Order::getID() const -> const std::string&
{
    return m_id;
}

auto Order::getStatus() const -> Order::Status
{
    return m_status;
}

auto Order::getStatusString() const -> std::string
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

auto Order::getTimestamp() const -> const std::chrono::system_clock::time_point&
{
    return m_timestamp;
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

void Order::setExecutedSize(int executedSize)
{
    m_executedSize = executedSize;
}

void Order::setPrice(double price)
{
    m_price = s_roundNearest(price, 0.01);
}

void Order::setExecutedPrice(double executedPrice)
{
    m_executedPrice = executedPrice;
}

void Order::setID(const std::string& id)
{
    m_id = id;
}

void Order::setStatus(Order::Status status)
{
    m_status = status;
}

void Order::setTimestamp()
{
    m_timestamp = std::chrono::system_clock::now();
}

} // shift
