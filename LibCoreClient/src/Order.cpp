#include "Order.h"

/**
 * @brief Default constructor for Order object.
 */
shift::Order::Order()
{
}

/**
 * @brief Constructor with all members preset.
 * @param symbol string value to be set in m_symbol
 * @param price double value for m_price
 * @param size int value for m_size
 * @param type char value for m_orderType
 */
shift::Order::Order(std::string symbol, double price, int size,
    ORDER_TYPE type)
    : m_symbol(std::move(symbol))
    , m_price(price)
    , m_size(size)
    , m_type(type)
{
    m_id = shift::crossguid::newGuid().str();
}

/**
 * @brief Constructor with all members preset.
 * @param symbol string value to be set in m_symbol
 * @param price double value for m_price
 * @param size int value for m_size
 * @param type char value for m_orderType
 */
shift::Order::Order(std::string symbol, double price, int size,
    ORDER_TYPE type, std::string id)
    : m_symbol(std::move(symbol))
    , m_price(price)
    , m_size(size)
    , m_type(type)
    , m_id(std::move(id))
{
}

/**
 * @brief Getter to get the symbol of current Order.
 * @return Symbol of the current Quote as a string.
 */
const std::string& shift::Order::getSymbol() const
{
    return m_symbol;
}

/**
 * @brief Getter to get the price of current Order.
 * @return Price of the current Order as a double.
 */
double shift::Order::getPrice() const
{
    return m_price;
}

/**
 * @brief Getter to get the size of current Order.
 * @return Size of the current Order as a int.
 */
int shift::Order::getSize() const
{
    return m_size;
}

/**
 * @brief Getter to get the type of current Order.
 * @return Type of the current Order as a char.
 */
shift::Order::ORDER_TYPE shift::Order::getType() const
{
    return m_type;
}

/**
 * @brief Getter to get the ID of current Order.
 * @return ID of the current Order as a string.
 */
const std::string& shift::Order::getID() const
{
    return m_id;
}

/**
 * @brief Setter to set Order symbol into m_symbol.
 * @param symbol as string
 */
void shift::Order::setSymbol(std::string symbol)
{
    m_symbol = std::move(symbol);
}

/**
 * @brief Setter to set order price into m_price.
 * @param price as double
 */
void shift::Order::setPrice(double price)
{
    m_price = price;
}

/**
 * @brief Setter to set order size into m_size.
 * @param size as int
 */
void shift::Order::setSize(int size)
{
    m_size = size;
}

/**
 * @brief Setter to set order type into m_orderType.
 * @param type as ORDER_TYPE
 */
void shift::Order::setType(ORDER_TYPE type)
{
    m_type = type;
}

/**
 * @brief Setter to set ID into m_id.
 * @param id as string
 */
void shift::Order::setID(std::string id)
{
    m_id = std::move(id);
}
