#include "Order.h"

Order::Order(std::string symbol, double price, int size, Order::Type type, std::string destination, FIX::UtcTimeStamp simulationTime)
    : m_symbol { std::move(symbol) }
    , m_traderID { "TRTH" }
    , m_orderID { "TRTH" }
    , m_milli { 0 }
    , m_price { price }
    , m_size { size }
    , m_type { type }
    , m_destination { std::move(destination) }
    , m_simulationTime { std::move(simulationTime) }
    , m_auctionCounter { 0 }
{
}

Order::Order(std::string symbol, std::string traderID, std::string orderID, double price, int size, Order::Type type, FIX::UtcTimeStamp simulationTime)
    : m_symbol { std::move(symbol) }
    , m_traderID { std::move(traderID) }
    , m_orderID { std::move(orderID) }
    , m_milli { 0 }
    , m_price(price)
    , m_size(size)
    , m_type(type)
    , m_destination("SHIFT")
    , m_simulationTime { std::move(simulationTime) }
    , m_auctionCounter { 0 }
{
}

auto Order::operator==(const Order& other) -> bool
{
    return m_symbol == other.m_symbol
        && m_traderID == other.m_traderID
        && m_orderID == other.m_orderID
        && m_price == other.m_price
        && m_size == other.m_size
        && m_type == other.m_type
        && m_destination == other.m_destination;
}

auto Order::getSymbol() const -> const std::string&
{
    return m_symbol;
}

auto Order::getTraderID() const -> const std::string&
{
    return m_traderID;
}

auto Order::getOrderID() const -> const std::string&
{
    return m_orderID;
}

auto Order::getMilli() const -> long
{
    return m_milli;
}

auto Order::getPrice() const -> double
{
    return m_price;
}

auto Order::getSize() const -> int
{
    return m_size;
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
    case TRTH_TRADE: {
        return "TRTH Trade";
    }
    case TRTH_BID: {
        return "TRTH Bid";
    }
    case TRTH_ASK: {
        return "TRTH Ask";
    }
    }
    return "N/A";
}

auto Order::getDestination() const -> const std::string&
{
    return m_destination;
}

auto Order::getTime() const -> const FIX::UtcTimeStamp&
{
    return m_simulationTime;
}

auto Order::getAuctionCounter() const -> int
{
    return m_auctionCounter;
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

void Order::incrementAuctionCounter()
{
    ++m_auctionCounter;
}
