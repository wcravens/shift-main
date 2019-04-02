#include "Quote.h"

/**
 * @brief Constructor for the Quote class.
 * @param stockName, traderID, orderID, price, size, time, orderType
 * @return None.
 */
Quote::Quote(std::string stockName,
    std::string traderID,
    std::string orderID,
    double price,
    int size,
    char orderType,
    FIX::UtcTimeStamp time)
    : m_stockName(std::move(stockName))
    , m_traderID(std::move(traderID))
    , m_orderID(std::move(orderID))
    , m_price(price)
    , m_size(size)
    , m_orderType(orderType)
    , m_destination("SHIFT")
    , m_time(std::move(time))
{
}

/**
 * @brief Constructor for the Quote class.
 * @param stockName, price, size, destination, time
 * @return None.
 */
Quote::Quote(std::string stockName,
    double price,
    int size,
    std::string destination,
    FIX::UtcTimeStamp time)
    : m_stockName(std::move(stockName))
    , m_traderID("TR")
    , m_orderID("Thomson")
    , m_price(price)
    , m_size(size)
    , m_orderType('1')
    , m_destination(std::move(destination))
    , m_time(std::move(time))
{
}

/**
 * @brief Constructor for the Quote class.
 * @param stockName, traderID, orderID, price, size, orderType
 * @return None.
 */
Quote::Quote(std::string stockName,
    std::string traderID,
    std::string orderID,
    double price,
    int size,
    char orderType)
    : m_stockName(std::move(stockName))
    , m_traderID(std::move(traderID))
    , m_orderID(std::move(orderID))
    , m_price(price)
    , m_size(size)
    , m_orderType(orderType)
    , m_destination("SHIFT")
{
}

/**
 * @brief Constructor for the Quote class.
 * @param the address of an exist Quote object
 * @return None.
 */
Quote::Quote(const Quote& other)
    : m_stockName(other.m_stockName)
    , m_traderID(other.m_traderID)
    , m_orderID(other.m_orderID)
    , m_milli(other.m_milli)
    , m_price(other.m_price)
    , m_size(other.m_size)
    , m_orderType(other.m_orderType)
    , m_destination(other.m_destination)
    , m_time(other.m_time)
{
}

/**
 * @brief Default constructor for the Quote class.
 * @param None.
 * @return None.
 */
Quote::Quote() {}

/**
 * @brief Default destructor for the Quote class.
 * @param None.
 * @return None.
 */
Quote::~Quote() {}

/**
 * @brief Overwrite function for the operator "=", make it compare two Quote objects
 * @param the address of an exist Quote object to be compared
 * @return None.
 */
void Quote::operator=(const Quote& other)
{
    m_stockName = other.m_stockName;
    m_traderID = other.m_traderID;
    m_orderID = other.m_orderID;
    m_milli = other.m_milli;
    m_price = other.m_price;
    m_size = other.m_size;
    m_orderType = other.m_orderType;
    m_destination = other.m_destination;
    m_time = other.m_time;
}

/**
 * @brief Setter for stockName field of current Quote object.
 * @param stockName to be set.
 * @return None.
 */
void Quote::setStockname(std::string stockName)
{
    m_stockName = std::move(stockName);
}

/**
 * @brief Getter for stockName field of current Quote object.
 * @param None.
 * @return stockName of the current Quote object.
 */
std::string Quote::getStockname()
{
    return m_stockName;
}

/**
 * @brief Getter for traderID field of current Quote object.
 * @param None.
 * @return traderID of the current Quote object.
 */
std::string Quote::getTraderID()
{
    return m_traderID;
}

/**
 * @brief Getter for orderID field of current Quote object.
 * @param None.
 * @return orderID of the current Quote object.
 */
std::string Quote::getOrderID()
{
    return m_orderID;
}

/**
 * @brief Setter for price field of current Quote object.
 * @param price to be set.
 * @return None.
 */
void Quote::setPrice(double price)
{
    m_price = price;
}

/**
 * @brief Getter for price field of current Quote object.
 * @param None.
 * @return price of the current Quote object.
 */
double Quote::getPrice()
{
    return m_price;
}

/**
 * @brief Setter for size field of current Quote object.
 * @param size to be set.
 * @return None.
 */
void Quote::setSize(int size)
{
    m_size = size;
}

/**
 * @brief Getter for size field of current Quote object.
 * @param None.
 * @return size of the current Quote object.
 */
int Quote::getSize()
{
    return m_size;
}

/**
 * @brief Getter for time field of current Quote object.
 * @param None.
 * @return time of the current Quote object.
 */
FIX::UtcTimeStamp Quote::getTime()
{
    return m_time;
}

/**
 * @brief Setter for orderType field of current Quote object.
 * @param orderType to be set.
 * @return None.
 */
void Quote::setOrderType(char orderType)
{
    m_orderType = orderType;
}

/**
 * @brief Getter for orderType field of current Quote object.
 * @param None.
 * @return orderType of the current Quote object.
 */
char Quote::getOrderType()
{
    return m_orderType;
}

/**
 * @brief Setter for mili field of current Quote object.
 * @param mili to be set.
 * @return None.
 */
void Quote::setMilli(long milli)
{
    m_milli = milli;
}

/**
 * @brief Getter for mili field of current Quote object.
 * @param None.
 * @return mili of the current Quote object.
 */
long Quote::getMilli()
{
    return m_milli;
}

/**
 * @brief Setter for destination field of current Quote object.
 * @param destination to be set.
 * @return None.
 */
void Quote::setDestination(std::string destination)
{
    m_destination = std::move(destination);
}

/**
 * @brief Getter for destination field of current Quote object.
 * @param None.
 * @return destination of the current Quote object.
 */
std::string Quote::getDestination()
{
    return m_destination;
}