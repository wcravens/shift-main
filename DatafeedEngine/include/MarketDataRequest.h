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

    const std::string& getRequestID() const;
    const std::vector<std::string>& getSymbols() const;
    const boost::posix_time::ptime& getStartTime() const;
    const boost::posix_time::ptime& getEndTime() const;
    int getNumSecondsPerDataChunk() const;
    std::string getDate() const;
    void updateStartTime(const boost::posix_time::ptime& newStartTime);

private:
    std::string m_requestID; ///> Field for request ID.
    std::vector<std::string> m_symbols; ///> Field for names of all requested tickers.
    boost::posix_time::ptime m_startTime; ///> Start time for the requested data.
    boost::posix_time::ptime m_endTime; ///> End time for the requested data.
    int m_numSecondsPerDataChunk = 300; ///> Send data chunk every 5min periodically by default.
};
