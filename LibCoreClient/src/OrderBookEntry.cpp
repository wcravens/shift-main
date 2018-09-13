#include "OrderBookEntry.h"

/**
 * @brief Default constructor for OrderBookEntry object.
 */
shift::OrderBookEntry::OrderBookEntry()
{
}

/**
 * @brief Constructor with all members preset.
 * @param type char value to be set in m_type
 * @param symbol string value to be set in m_symbol
 * @param price double value for m_price
 * @param size int value for m_size
 * @param destination string value for m_destination
 * @param time double value for m_time
 */
shift::OrderBookEntry::OrderBookEntry(char type, std::string symbol, double price, int size,
    std::string destination, double time)
    : m_type(type)
    , m_symbol(std::move(symbol))
    , m_price(price)
    , m_size(size)
    , m_destination(std::move(destination))
    , m_time(time)
{
}

/**
 * @brief Getter to get the type of current OrderBookEntry.
 * @return Type of the current OrderBookEntry as a char.
 */
char shift::OrderBookEntry::getType() const
{
    return m_type;
}

/**
 * @brief Getter to get the symbol of current OrderBookEntry.
 * @return Symbol of the current OrderBookEntry as a string.
 */
const std::string& shift::OrderBookEntry::getSymbol() const
{
    return m_symbol;
}

/**
 * @brief Getter to get the price of current OrderBookEntry.
 * @return Price of the current OrderBookEntry as a double.
 */
double shift::OrderBookEntry::getPrice() const
{
    return m_price;
}

/**
 * @brief Getter to get the size of current OrderBookEntry.
 * @return Size of the current OrderBookEntry as a double.
 */
int shift::OrderBookEntry::getSize() const
{
    return m_size;
}

/**
 * @brief Getter to get the destination of current OrderBookEntry.
 * @return Destination of the current OrderBookEntry as a string.
 */
const std::string& shift::OrderBookEntry::getDestination() const
{
    return m_destination;
}

/**
 * @brief Getter to get the execution time of current OrderBookEntry.
 * @return Execution time of the current OrderBookEntry as a double.
 */
double shift::OrderBookEntry::getTime() const
{
    return m_time;
}

/**
 * @brief Setter to set order type into m_type.
 * @param type in char
 */
void shift::OrderBookEntry::setType(char type)
{
    m_type = type;
}

/**
 * @brief Setter to set order symbol into m_symbol.
 * @param symbol in string
 */
void shift::OrderBookEntry::setSymbol(std::string symbol)
{
    m_symbol = std::move(symbol);
}

/**
 * @brief Setter to set order price into m_price.
 * @param price in double
 */
void shift::OrderBookEntry::setPrice(double price)
{
    m_price = price;
}

/**
 * @brief Setter to set order size into m_size.
 * @param size in double
 */
void shift::OrderBookEntry::setSize(int size)
{
    m_size = size;
}

/**
 * @brief Setter to set order destination into m_destination.
 * @param destination in string
 */
void shift::OrderBookEntry::setDestination(std::string destination)
{
    m_destination = destination;
}

/**
 * @brief Setter to set order time into m_time.
 * @param time in double (timestamp)
 */
void shift::OrderBookEntry::setTime(double time)
{
    m_time = time;
}
