#include "Order.h"

/**
 * @brief Default constructor for Order object.
 */
shift::Order::Order()
{
}

/**
 * @brief Constructor with all members preset.
 * @param type Type value for m_type
 * @param symbol string value to be set in m_symbol
 * @param size int value for m_size
 * @param price double value for m_price
 */
shift::Order::Order(shift::Order::Type type, const std::string& symbol, int size, double price, const std::string& id)
    : m_type(type)
    , m_symbol(symbol)
    , m_size(size)
    , m_price(price)
    , m_id(id)
{
    if (m_price <= 0.0) {
        m_price = 0.0;
        if (m_type == shift::Order::Type::LIMIT_BUY) {
            m_type = shift::Order::Type::MARKET_BUY;
        } else if (m_type == shift::Order::Type::LIMIT_SELL) {
            m_type = shift::Order::Type::MARKET_SELL;
        }
    }

    if (m_id.empty()) {
        m_id = shift::crossguid::newGuid().str();
    }
}

/**
 * @brief Getter to get the type of current Order.
 * @return Type of the current Order as a char.
 */
shift::Order::Type shift::Order::getType() const
{
    return m_type;
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
 * @brief Getter to get the size of current Order.
 * @return Size of the current Order as a int.
 */
int shift::Order::getSize() const
{
    return m_size;
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
 * @brief Getter to get the ID of current Order.
 * @return ID of the current Order as a string.
 */
const std::string& shift::Order::getID() const
{
    return m_id;
}

/**
 * @brief Setter to set order type into m_type.
 * @param type as Type
 */
void shift::Order::setType(Type type)
{
    m_type = type;
}

/**
 * @brief Setter to set Order symbol into m_symbol.
 * @param symbol as string
 */
void shift::Order::setSymbol(const std::string& symbol)
{
    m_symbol = symbol;
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
 * @brief Setter to set order price into m_price.
 * @param price as double
 */
void shift::Order::setPrice(double price)
{
    m_price = price;
}

/**
 * @brief Setter to set ID into m_id.
 * @param id as string
 */
void shift::Order::setID(const std::string& id)
{
    m_id = id;
}
