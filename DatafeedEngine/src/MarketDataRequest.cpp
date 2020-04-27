#include "MarketDataRequest.h"

#include <boost/date_time/posix_time/conversion.hpp>

/**
 * @brief Constructor for a new MarketDataRequest.
 * @param requestID, vector of ticker symbols, start time, and end time.
 */
MarketDataRequest::MarketDataRequest(std::string&& requestID, std::vector<std::string>&& symbols, boost::posix_time::ptime&& startTime, boost::posix_time::ptime&& endTime, int numSecondsPerDataChunk)
    : m_requestID { std::move(requestID) }
    , m_symbols { std::move(symbols) }
    , m_startTime { std::move(startTime) }
    , m_endTime { std::move(endTime) }
    , m_numSecondsPerDataChunk { numSecondsPerDataChunk }
{
}

/**
 * @brief Method to get requestID for current MarketDataRequest.
 * @return The request id for the current MarketDataRequest.
 */
auto MarketDataRequest::getRequestID() const -> const std::string&
{
    return m_requestID;
}

/**
 * @brief Method to get a list of symbols for current MarketDataRequest.
 * @return A vector with all symbols.
 */
auto MarketDataRequest::getSymbols() const -> const std::vector<std::string>&
{
    return m_symbols;
}

/**
 * @brief Method to get start time for current MarketDataRequest.
 * @return Start time for the current MarketDataRequest
 */
auto MarketDataRequest::getStartTime() const -> const boost::posix_time::ptime&
{
    return m_startTime;
}

/**
 * @brief Method to get end time for current MarketDataRequest.
 * @return End time for the current MarketDataRequest
 */
auto MarketDataRequest::getEndTime() const -> const boost::posix_time::ptime&
{
    return m_endTime;
}

auto MarketDataRequest::getNumSecondsPerDataChunk() const -> int
{
    return m_numSecondsPerDataChunk;
}

/**
 * @brief Method to get requested date for current MarketDataRequest.
 * @return Requested date for the current MarketDataRequest
 */
auto MarketDataRequest::getDate() const -> std::string
{
    std::string date = boost::posix_time::to_iso_extended_string(m_startTime);
    return date.substr(0, 10);
}

void MarketDataRequest::updateStartTime(const boost::posix_time::ptime& newStartTime)
{
    m_startTime = newStartTime;
}
