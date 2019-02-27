#include "Quote.h"

/**
*   @brief  Default constructor of Quote.
*/
Quote::Quote() = default;

/**
*   @brief  Constructs with corresponding parameters.
*   @param  symbol: The stock name of quote
*   @param  username： The username of quote
*   @param  orderID: The order ID of quote
*   @param  price: The price of quote
*   @param  shareSize: The size of quote
*   @param  orderType: The order type of quotes
*/
Quote::Quote(std::string symbol, std::string username, std::string orderID, double price, int shareSize, ORDER_TYPE orderType)
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
const std::string& Quote::getSymbol() const
{
    return m_symbol;
}

/**
*   @brief  Getter of username.
*   @return The username.
*/
const std::string& Quote::getUsername() const
{
    return m_username;
}

/**
*   @brief  Getter of order ID.
*   @return The order ID.
*/
const std::string& Quote::getOrderID() const
{
    return m_orderID;
}

/**
*   @brief  Setter of price of the quote.
*   @return nothing
*/
void Quote::setPrice(double price)
{
    m_price = price;
}

/**
*   @brief  Getter of price.
*   @return The price.
*/
double Quote::getPrice() const
{
    return m_price;
}

/**
*   @brief  Setter of share size of the quote.
*   @return nothing
*/
void Quote::setShareSize(int shareSize)
{
    m_shareSize = shareSize;
}

/**
*   @brief  Getter of share size.
*   @return The share size.
*/
int Quote::getShareSize() const
{
    return m_shareSize;
}

/**
*   @brief  Getter of order type.
*   @return The order type.
*/
auto Quote::getOrderType() const -> ORDER_TYPE
{
    return m_orderType;
}
