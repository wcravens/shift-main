#pragma once

#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

/**
 * @brief class to save Market Data Request information from Matching Engine.
 */
class MarketDataRequest {
private:
    std::string m_requestID; // Field for request ID
    std::vector<std::string> m_symbols; // Field for names of all requested tickers
    boost::posix_time::ptime m_timeStart; // Start time for the requested data
    boost::posix_time::ptime m_timeEnd; // End time for the requested data

public:
    const std::string& getRequestID() const;
    const std::vector<std::string>& getSymbols() const;
    const boost::posix_time::ptime& getStartTime() const;
    const boost::posix_time::ptime& getEndTime() const;
    std::string getDate() const;
    void updateTimeStart(boost::posix_time::ptime newStartTime);

    MarketDataRequest() = default;
    MarketDataRequest(std::string requestID, std::vector<std::string> symbols, boost::posix_time::ptime timeStart, boost::posix_time::ptime timeEnd);
};
