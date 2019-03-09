#include "Order.h"

/**
*   @brief  Default constructor of Order.
*/
Order::Order() = default;

/**
*   @brief  Constructs with corresponding parameters.
*   @param  symbol: The stock name of order
*   @param  username： The username of order
*   @param  orderID: The order ID of order
*   @param  price: The price of order
*   @param  shareSize: The size of order
*   @param  orderType: The order type of orders
*/
Order::Order(std::string symbol, std::string username, std::string orderID, double price, int shareSize, ORDER_TYPE orderType)
    : m_symbol(std::move(symbol))
    , m_username(std::move(username))
    , m_orderID(std::move(orderID))
    , m_price{ price }
    , m_shareSize(shareSize)
    , m_orderType(orderType)
{
}

/**
*   @brief  Getter of stock name.
*   @return The symbol.
*/
const std::string& Order::getSymbol() const
{
    return m_symbol;
}

/**
*   @brief  Getter of username.
*   @return The username.
*/
const std::string& Order::getUsername() const
{
    return m_username;
}

/**
*   @brief  Getter of order ID.
*   @return The order ID.
*/
const std::string& Order::getOrderID() const
{
    return m_orderID;
}

/**
*   @brief  Setter of price of the order.
*   @return nothing
*/
void Order::setPrice(double price)
{
    m_price = price;
}

/**
*   @brief  Getter of price.
*   @return The price.
*/
double Order::getPrice() const
{
    return m_price;
}

/**
*   @brief  Setter of share size of the order.
*   @return nothing
*/
void Order::setShareSize(int shareSize)
{
    m_shareSize = shareSize;
}

/**
*   @brief  Getter of share size.
*   @return The share size.
*/
int Order::getShareSize() const
{
    return m_shareSize;
}

/**
*   @brief  Getter of order type.
*   @return The order type.
*/
auto Order::getOrderType() const -> ORDER_TYPE
{
    return m_orderType;
}
