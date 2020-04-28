#include "Order.h"

Order::Order(Order::Type type, std::string symbol, int size, double price, std::string id, std::string userID)
    : m_type { type }
    , m_symbol { std::move(symbol) }
    , m_size { size }
    , m_executedSize { 0 }
    , m_price { price }
    , m_id { std::move(id) }
    , m_userID { std::move(userID) }
    , m_status { Order::Status::PENDING_NEW }
{
}

/* static */ auto Order::s_typeToString(Type type) -> std::string
{
    switch (type) {
    case Type::LIMIT_BUY:
        return "LIMIT_BUY";
    case Type::LIMIT_SELL:
        return "LIMIT_SELL";
    case Type::MARKET_BUY:
        return "MARKET_BUY";
    case Type::MARKET_SELL:
        return "MARKET_SELL";
    case Type::CANCEL_BID:
        return "CANCEL_BID";
    case Type::CANCEL_ASK:
        return "CANCEL_ASK";
    case Type::TRTH_TRADE:
        return "TRTH_TRADE";
    case Type::TRTH_BID:
        return "TRTH_BID";
    case Type::TRTH_ASK:
        return "TRTH_ASK";
    default:;
    }
    return "UNKNOWN";
}

auto Order::getType() const -> Order::Type
{
    return m_type;
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

auto Order::getID() const -> const std::string&
{
    return m_id;
}

auto Order::getUserID() const -> const std::string&
{
    return m_userID;
}

auto Order::getStatus() const -> Order::Status
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

void Order::setExecutedSize(int executedSize)
{
    m_executedSize = executedSize;
}

void Order::setPrice(double price)
{
    m_price = price;
}

void Order::setID(const std::string& id)
{
    m_id = id;
}

void Order::setUserID(const std::string& userID)
{
    m_userID = userID;
}

void Order::setStatus(Order::Status status)
{
    m_status = status;
}
