#include "Report.h"

/**
 * @brief Default constructor for Report object.
 */
shift::Report::Report()
{
}

/**
 * @brief Constructor with all members preset.
 * @param clientID string value to be set in m_clientID
 * @param orderID string value to be set in m_orderID
 * @param symbol string value to be set in m_symbol
 * @param price double value for m_price
 * @param orderType char value for m_orderType
 * @param orderStatus char value for m_orderStatus
 * @param size int value for m_size
 * @param executionTime string value for m_executionTime
 * @param serverTime string value for m_serverTime
 */
shift::Report::Report(const std::string& clientID, const std::string& orderID, const std::string& symbol, double price, char orderType,
    char orderStatus, int size, const std::string& executionTime, const std::string& serverTime)
    : m_clientID(clientID)
    , m_orderID(orderID)
    , m_symbol(symbol)
    , m_price(price)
    , m_orderType(orderType)
    , m_orderStatus(orderStatus)
    , m_size(size)
    , m_executionTime(executionTime)
    , m_serverTime(serverTime)
{
}
