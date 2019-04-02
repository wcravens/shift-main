#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

class Quote {

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

public:
    friend class Stock;
    //for the server to receive
    Quote(std::string stockName,
        std::string traderID,
        std::string orderID,
        double price,
        int size,
        char orderType,
        FIX::UtcTimeStamp time);

    Quote(std::string stockName,
        double price,
        int size,
        std::string destination,
        FIX::UtcTimeStamp time);

    // for the client to send
    Quote(std::string stockName,
        std::string traderID,
        std::string orderID,
        double price,
        int size,
        char orderType);

    Quote();
    Quote(const Quote& newquote);
    ~Quote();

    void operator=(const Quote& newquote);

    void setStockname(std::string name);
    std::string getStockname();

    std::string getTraderID();

    std::string getOrderID();

    void setMilli(long milli);
    long getMilli();

    void setPrice(double price);
    double getPrice();

    void setSize(int size);
    int getSize();

    FIX::UtcTimeStamp getTime();

    void setOrderType(char ordertype);
    char getOrderType();

    void setDestination(std::string destination);
    std::string getDestination();
};
