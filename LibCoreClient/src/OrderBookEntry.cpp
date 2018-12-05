#include "OrderBookEntry.h"

/**
 * @brief Default constructor for OrderBookEntry object.
 */
shift::OrderBookEntry::OrderBookEntry()
    : m_price(0.0)
    , m_size(0)
    , m_time(0.0)
    , m_destination("")
{
}

/**
 * @brief Constructor with all members preset.
 * @param price double value for m_price
 * @param size int value for m_size
 * @param destination string value for m_destination
 * @param time double value for m_time
 */
shift::OrderBookEntry::OrderBookEntry(double price, int size, double time, const std::string& destination)
    : m_price(price)
    , m_size(size)
    , m_time(time)
    , m_destination(destination)
{
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
 * @brief Getter to get the execution time of current OrderBookEntry.
 * @return Time of the current OrderBookEntry as a double.
 */
double shift::OrderBookEntry::getTime() const
{
    return m_time;
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
 * @brief Setter to set order price into m_price.
 * @param price as double
 */
void shift::OrderBookEntry::setPrice(double price)
{
    m_price = price;
}

/**
 * @brief Setter to set order size into m_size.
 * @param size as double
 */
void shift::OrderBookEntry::setSize(int size)
{
    m_size = size;
}

/**
 * @brief Setter to set order time into m_time.
 * @param time as double (timestamp)
 */
void shift::OrderBookEntry::setTime(double time)
{
    m_time = time;
}

/**
 * @brief Setter to set order destination into m_destination.
 * @param destination as string
 */
void shift::OrderBookEntry::setDestination(const std::string& destination)
{
    m_destination = destination;
}