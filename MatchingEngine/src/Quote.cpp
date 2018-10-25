#include "Quote.h"

/**
 * @brief Constructor for the Quote class.
 * @param stockname, traderID, orderID, price, size, time, orderType
 * @return None.
 */
Quote::Quote(std::string stockname1, 
            std::string trader_id1, 
            std::string order_id1, 
            double price1, 
            int size1, 
            char ordertype1, 
            FIX::UtcTimeStamp utc
)
{
    stockname = stockname1;
    trader_id = trader_id1;
    order_id = order_id1;
    price = price1;
    size = size1;
    // time = time1;
    ordertype = ordertype1;
    destination = "Server";
    utc_time = utc;
}

Quote::Quote(std::string stockname1, 
            double price1, 
            int size1, 
            std::string destination1, 
            FIX::UtcTimeStamp utc
)
{
    stockname = stockname1;
    trader_id = "TR";
    order_id = "Thomson";
    price = price1;
    size = size1;
    // time = time1;
    ordertype = '1';
    destination = destination1;
    utc_time = utc;
}

/**
 * @brief Constructor for the Quote class.
 * @param stockname, traderID, orderID, price, size, orderType
 * @return None.
 */
Quote::Quote(std::string stockname1, 
            std::string trader_id1, 
            std::string order_id1, 
            double price1, 
            int size1, 
            char ordertype1
)
{
    stockname = stockname1;
    stockname = stockname1;
    trader_id = trader_id1;
    order_id = order_id1;
    price = price1;
    size = size1;
    ordertype = ordertype1;
    destination = "server";
}

/**
 * @brief Constructor for the Quote class.
 * @param the address of an exist Quote object
 * @return None.
 */
Quote::Quote(const Quote& newquote)
{
    stockname = newquote.stockname;
    trader_id = newquote.trader_id;
    order_id = newquote.order_id;
    mili = newquote.mili;
    price = newquote.price;
    size = newquote.size;
    // time = newquote.time;
    ordertype = newquote.ordertype;
    destination = newquote.destination;
    utc_time = newquote.utc_time;
}

/**
 * @brief Default constructor for the Quote class.
 * @param None.
 * @return None.
 */
Quote::Quote(void)
{
}

/**
 * @brief Default destructor for the Quote class.
 * @param None.
 * @return None.
 */
Quote::~Quote(void)
{
}

/**
 * @brief Overwrite function for the operator "=", make it compare two Quote objects
 * @param the address of an exist Quote object to be compared
 * @return None.
 */
void Quote::operator=(const Quote& newquote)
{
    stockname = newquote.stockname;
    trader_id = newquote.trader_id;
    order_id = newquote.order_id;
    mili = newquote.mili;
    price = newquote.price;
    size = newquote.size;
    // time = newquote.time;
    ordertype = newquote.ordertype;
    destination = newquote.destination;
    utc_time = newquote.utc_time;
}

/**
 * @brief Setter for stockname field of current Quote object.
 * @param stockname to be set.
 * @return None.
 */
void Quote::setstockname(std::string name1)
{
    stockname = name1;
}

/**
 * @brief Getter for stockname field of current Quote object.
 * @param None.
 * @return stockname of the current Quote object.
 */
std::string Quote::getstockname()
{
    return stockname;
}

/**
 * @brief Setter for traderID field of current Quote object.
 * @param traderID to be set.
 * @return None.
 */
void Quote::settrader_id(std::string trader_id1)
{
    trader_id = trader_id1;
}

/**
 * @brief Getter for traderID field of current Quote object.
 * @param None.
 * @return traderID of the current Quote object.
 */
std::string Quote::gettrader_id()
{
    return trader_id;
}

/**
 * @brief Setter for orderID field of current Quote object.
 * @param orderID to be set.
 * @return None.
 */
void Quote::setorder_id(std::string order_id1)
{
    order_id = order_id1;
}

/**
 * @brief Getter for orderID field of current Quote object.
 * @param None.
 * @return orderID of the current Quote object.
 */
std::string Quote::getorder_id()
{
    return order_id;
}

/**
 * @brief Setter for price field of current Quote object.
 * @param price to be set.
 * @return None.
 */
void Quote::setprice(double price1)
{
    price = price1;
}

/**
 * @brief Getter for price field of current Quote object.
 * @param None.
 * @return price of the current Quote object.
 */
double Quote::getprice()
{
    return price;
}

/**
 * @brief Setter for size field of current Quote object.
 * @param size to be set.
 * @return None.
 */
void Quote::setsize(int size1)
{
    size = size1;
}

/**
 * @brief Getter for size field of current Quote object.
 * @param None.
 * @return size of the current Quote object.
 */
int Quote::getsize()
{
    return size;
}

/**
 * @brief Setter for time field of current Quote object.
 * @param time to be set.
 * @return None.
 */
// void Quote::settime(std::string time1)
// {
//     time = time1;
// }

/**
 * @brief Getter for time field of current Quote object.
 * @param None.
 * @return time of the current Quote object.
 */
// std::string Quote::gettime()
// {
//     return time;
// }

FIX::UtcTimeStamp Quote::getutctime() {
    return utc_time;
}

/**
 * @brief Setter for orderType field of current Quote object.
 * @param orderType to be set.
 * @return None.
 */
void Quote::setordertype(char ordertype1)
{
    ordertype = ordertype1;
}

/**
 * @brief Getter for orderType field of current Quote object.
 * @param None.
 * @return orderType of the current Quote object.
 */
char Quote::getordertype()
{
    return ordertype;
}
