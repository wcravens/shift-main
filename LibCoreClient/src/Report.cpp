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
shift::Report::Report(std::string clientID, std::string orderID, std::string symbol, double price, char orderType,
    char orderStatus, int size, std::string executionTime, std::string serverTime)
    : m_clientID(std::move(clientID))
    , m_orderID(std::move(orderID))
    , m_symbol(std::move(symbol))
    , m_price(price)
    , m_orderType(orderType)
    , m_orderStatus(orderStatus)
    , m_size(size)
    , m_executionTime(std::move(executionTime))
    , m_serverTime(std::move(serverTime))
{
}
