#pragma once

#include <string>

class Report {

private:
    std::string m_orderID;
    std::string m_clientID;
    std::string m_ticker;
    double m_price;
    char m_orderType;
    char m_status; // order status: '1' : confirmed; '2' : partially filled; '3': canceled
    int m_executedQuantity; // the amount of executed shares
    int m_remainingQuantity; // the amount of remaining shares
    std::string m_time;

public:
    Report()
    {
    }

    Report(std::string orderID, std::string clientID, std::string ticker, double price, char orderType, char status, int executedQuantity, int remainingQuantity, std::string time)
        : m_orderID(std::move(orderID))
        , m_clientID(std::move(clientID))
        , m_ticker(std::move(ticker))
        , m_price(price)
        , m_orderType(orderType)
        , m_status(status)
        , m_executedQuantity(executedQuantity)
        , m_remainingQuantity(remainingQuantity)
        , m_time(std::move(time))
    {
    }

    Report(std::string orderID, char status, int executedQuantity, int remainingQuantity, std::string time) // For partially filled
        : m_orderID(std::move(orderID))
        , m_status(status)
        , m_executedQuantity(executedQuantity)
        , m_remainingQuantity(remainingQuantity)
        , m_time(std::move(time))
    {
    }

    Report(std::string orderID, char status) // For confirmed, canceled and filled
        : m_orderID(std::move(orderID))
        , m_status(status)
    {
    }

    ~Report()
    {
    }

    void setOrderID(std::string orderID)
    {
        m_orderID = orderID;
    }

    std::string getOrderID()
    {
        return m_orderID;
    }

    void setClientID(std::string clientID)
    {
        m_clientID = clientID;
    }

    std::string getClientID()
    {
        return m_clientID;
    }

    void setTicker(std::string ticker)
    {
        m_ticker = ticker;
    }

    std::string getTicker()
    {
        return m_ticker;
    }

    void setPrice(double price)
    {
        m_price = price;
    }

    double getPrice()
    {
        return m_price;
    }

    void setOrdertype(char orderType)
    {
        m_orderType = orderType;
    }

    char getOrdertype()
    {
        return m_orderType;
    }

    void setStatus(char status)
    {
        m_status = status;
    }
    char getStatus()
    {
        return m_status;
    }

    void setExecutedQuantity(int executedQuantity)
    {
        m_executedQuantity = executedQuantity;
    }

    int getExecutedQuantity()
    {
        return m_executedQuantity;
    }

    void setRemainingQuantity(int remainingQuantity)
    {
        m_remainingQuantity = remainingQuantity;
    }

    int getRemainingQuantity()
    {
        return m_remainingQuantity;
    }

    void setTime(std::string time)
    {
        m_time = time;
    }

    std::string getTime()
    {
        return m_time;
    }
};
