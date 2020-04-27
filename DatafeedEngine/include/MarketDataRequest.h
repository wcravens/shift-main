#pragma once

#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

/**
 * @brief Class to save Market Data Request information from Matching Engine.
 */
class MarketDataRequest {
public:
    MarketDataRequest() = default;
    MarketDataRequest(std::string&& requestID, std::vector<std::string>&& symbols, boost::posix_time::ptime&& startTime, boost::posix_time::ptime&& endTime, int numSecondsPerDataChunk);

    auto getRequestID() const -> const std::string&;
    auto getSymbols() const -> const std::vector<std::string>&;
    auto getStartTime() const -> const boost::posix_time::ptime&;
    auto getEndTime() const -> const boost::posix_time::ptime&;
    auto getNumSecondsPerDataChunk() const -> int;
    auto getDate() const -> std::string;
    void updateStartTime(const boost::posix_time::ptime& newStartTime);

private:
    std::string m_requestID; ///> Field for request ID.
    std::vector<std::string> m_symbols; ///> Field for names of all requested tickers.
    boost::posix_time::ptime m_startTime; ///> Start time for the requested data.
    boost::posix_time::ptime m_endTime; ///> End time for the requested data.
    int m_numSecondsPerDataChunk = 300; ///> Send data chunk every 5min periodically by default.
};
