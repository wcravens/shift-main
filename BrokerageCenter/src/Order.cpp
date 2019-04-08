#include "Order.h"

Order::Order(Order::Type type, const std::string& symbol, int size, double price, const std::string& id, const std::string& userID)
    : m_type(type)
    , m_symbol(symbol)
    , m_size(size)
    , m_executedSize(0)
    , m_price(price)
    , m_id(id)
    , m_userID(userID)
    , m_status(Order::Status::PENDING_NEW)
{
}

Order::Type Order::getType() const
{
    return m_type;
}

/*static*/ const char* Order::s_typeToString(Type type)
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
    default:;
    }
    return "UNKNOWN";
}

const std::string& Order::getSymbol() const
{
    return m_symbol;
}

int Order::getSize() const
{
    return m_size;
}

int Order::getExecutedSize() const
{
    return m_executedSize;
}

double Order::getPrice() const
{
    return m_price;
}

const std::string& Order::getID() const
{
    return m_id;
}

const std::string& Order::getUserID() const
{
    return m_userID;
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
