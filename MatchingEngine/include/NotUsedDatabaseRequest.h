#pragma once

#include <string>

// API: Requests sent from Server to Database
class DatabaseRequest {

private:
    std::string m_requestID;
    char m_type; // '1' means sending quotes and trades; '2' means storing data; '3' means checking records
    std::string m_ticker;
    std::string m_date;
    std::string m_startTime;
    std::string m_endTime;

public:
    DatabaseRequest()
    {
    }

    DatabaseRequest(std::string requestID, char type, std::string ticker, std::string date, std::string startTime, std::string endTime)
        : m_requestID(std::move(requestID))
        , m_type(type)
        , m_ticker(std::move(ticker))
        , m_date(std::move(date))
        , m_startTime(std::move(startTime))
        , m_endTime(std::move(endTime))
    {
    }

    ~DatabaseRequest()
    {
    }

    void setRequestID(std::string requestID)
    {
        m_requestID = std::move(requestID);
    }

    std::string getRequestID()
    {
        return m_requestID;
    }

    void setType(char type)
    {
        m_type = type;
    }

    char getType()
    {
        return m_type;
    }

    void setSymbol(std::string ticker)
    {
        m_ticker = std::move(ticker);
    }

    std::string getSymbol()
    {
        return m_ticker;
    }

    void setDate(std::string date)
    {
        m_date = std::move(date);
    }

    std::string getDate()
    {
        return m_date;
    }

    void setStartTime(std::string startTime)
    {
        m_startTime = std::move(startTime);
    }

    std::string getStartTime()
    {
        return m_startTime;
    }

    void setEndTime(std::string endTime)
    {
        m_endTime = std::move(endTime);
    }

    std::string getEndTime()
    {
        return m_endTime;
    }
};
