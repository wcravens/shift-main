#include "MarketDataRequest.h"

#include <boost/date_time/posix_time/conversion.hpp>

/**
 * @brief Constructor for a new MarketDataRequest.
 * @param requestID, vector of ticker symbols, start time, and end time.
 */
MarketDataRequest::MarketDataRequest(std::string requestID, std::vector<std::string> symbols, boost::posix_time::ptime timeStart, boost::posix_time::ptime timeEnd)
    : m_requestID(std::move(requestID))
    , m_symbols(std::move(symbols))
    , m_timeStart(std::move(timeStart))
    , m_timeEnd(std::move(timeEnd))
{
}

/**
 * @brief method to get requestID for current MarketDataRequest
 * @param None
 * @return the request id for the current MarketDataRequest.
 */
const std::string& MarketDataRequest::getRequestID() const
{
    return m_requestID;
}

/**
 * @brief method to get a list of symbols for current MarketDataRequest
 * @param None
 * @return a vector with all symbols.
 */
const std::vector<std::string>& MarketDataRequest::getSymbols() const
{
    return m_symbols;
}

/**
 * @brief method to get start time for current MarketDataRequest
 * @param None
 * @return start time for the current MarketDataRequest
 */
const boost::posix_time::ptime& MarketDataRequest::getStartTime() const
{
    return m_timeStart;
}

/**
 * @brief method to get end time for current MarketDataRequest
 * @param None
 * @return end time for the current MarketDataRequest
 */
const boost::posix_time::ptime& MarketDataRequest::getEndTime() const
{
    return m_timeEnd;
}

/**
 * @brief method to get requested date for current MarketDataRequest
 * @param None
 * @return requested date for the current MarketDataRequest
 */
std::string MarketDataRequest::getDate() const
{
    std::string date = boost::posix_time::to_iso_extended_string(m_timeStart);
    return date.substr(0, 10);
}

void MarketDataRequest::updateTimeStart(boost::posix_time::ptime newStartTime)
{
    m_timeStart = std::move(newStartTime);
}
