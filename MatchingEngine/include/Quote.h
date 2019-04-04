#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

class Quote {
public:
    Quote() = default;
    Quote(const std::string& stockName, const std::string& traderID, const std::string& orderID, double price, int size, char orderType, const FIX::UtcTimeStamp& time);
    Quote(const std::string& stockName, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);

    // Getters
    const std::string& getStockName() const;
    const std::string& getTraderID() const;
    const std::string& getOrderID() const;
    long getMilli() const;
    double getPrice() const;
    int getSize() const;
    char getOrderType() const;
    const std::string& getDestination() const;
    const FIX::UtcTimeStamp& getTime() const;

    // Setters
    void setStockName(const std::string& stockName);
    void setMilli(long milli);
    void setPrice(double price);
    void setSize(int size);
    void setOrderType(char orderType);
    void setDestination(const std::string& destination);

private:
    std::string m_stockName;
    std::string m_traderID;
    std::string m_orderID;
    long m_milli;
    double m_price;
    int m_size;
    char m_orderType;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
