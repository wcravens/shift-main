#include "Quote.h"

/**
 * @brief Constructor for the Quote class.
 * @param stockname, traderID, orderID, price, size, time, orderType
 * @return None.
 */
Quote::Quote(std::string _stockname,
    std::string _traderID,
    std::string _orderID,
    double _price,
    int _size,
    char _orderType,
    FIX::UtcTimeStamp _time)
    : m_stockname(std::move(_stockname))
    , m_traderID(std::move(_traderID))
    , m_orderID(std::move(_orderID))
    , m_price(_price)
    , m_size(_size)
    , m_orderType(_orderType)
    , m_destination("SHIFT")
    , m_time(std::move(_time))
{
}

/**
 * @brief Constructor for the Quote class.
 * @param stockname, price, size, destination, time
 * @return None.
 */
Quote::Quote(std::string _stockname,
    double _price,
    int _size,
    std::string _destination,
    FIX::UtcTimeStamp _time)
    : m_stockname(std::move(_stockname))
    , m_traderID("TR")
    , m_orderID("Thomson")
    , m_price(_price)
    , m_size(_size)
    , m_orderType('1')
    , m_destination(std::move(_destination))
    , m_time(std::move(_time))
{
}

/**
 * @brief Constructor for the Quote class.
 * @param stockname, traderID, orderID, price, size, orderType
 * @return None.
 */
Quote::Quote(std::string _stockname,
    std::string _traderID,
    std::string _orderID,
    double _price,
    int _size,
    char _orderType)
    : m_stockname(std::move(_stockname))
    , m_traderID(std::move(_traderID))
    , m_orderID(std::move(_orderID))
    , m_price(_price)
    , m_size(_size)
    , m_orderType(_orderType)
    , m_destination("SHIFT")
{
}

/**
 * @brief Constructor for the Quote class.
 * @param the address of an exist Quote object
 * @return None.
 */
Quote::Quote(const Quote& newquote)
    : m_stockname(newquote.m_stockname)
    , m_traderID(newquote.m_traderID)
    , m_orderID(newquote.m_orderID)
    , m_milli(newquote.m_milli)
    , m_price(newquote.m_price)
    , m_size(newquote.m_size)
    , m_orderType(newquote.m_orderType)
    , m_destination(newquote.m_destination)
    , m_time(newquote.m_time)
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
void Quote::operator=(const Quote& newquote)
{
    m_stockname = newquote.m_stockname;
    m_traderID = newquote.m_traderID;
    m_orderID = newquote.m_orderID;
    m_milli = newquote.m_milli;
    m_price = newquote.m_price;
    m_size = newquote.m_size;
    m_orderType = newquote.m_orderType;
    m_destination = newquote.m_destination;
    m_time = newquote.m_time;
}

/**
 * @brief Setter for stockname field of current Quote object.
 * @param stockname to be set.
 * @return None.
 */
void Quote::setStockname(std::string _stockname)
{
    m_stockname = std::move(_stockname);
}

/**
 * @brief Getter for stockname field of current Quote object.
 * @param None.
 * @return stockname of the current Quote object.
 */
std::string Quote::getStockname()
{
    return m_stockname;
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
void Quote::setPrice(double _price)
{
    m_price = _price;
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
void Quote::setSize(int _size)
{
    m_size = _size;
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
void Quote::setOrderType(char _orderType)
{
    m_orderType = _orderType;
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
void Quote::setMilli(long _milli)
{
    m_milli = _milli;
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
void Quote::setDestination(std::string _destination)
{
    m_destination = std::move(_destination);
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