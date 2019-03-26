#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

class Quote {

private:
    std::string m_stockname;
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
    Quote(std::string _stockname,
        std::string _traderID,
        std::string _orderID,
        double _price,
        int _size,
        char _orderType,
        FIX::UtcTimeStamp _time);

    Quote(std::string _stockname,
        double _price,
        int _size,
        std::string _destination,
        FIX::UtcTimeStamp _time);

    // for the client to send
    Quote(std::string _stockname,
        std::string _traderID,
        std::string _orderID,
        double _price,
        int _size,
        char _orderType);

    Quote(const Quote& _newquote);

    Quote();
    ~Quote();

    void operator=(const Quote& newquote);

    void setStockname(std::string name1);
    std::string getStockname();

    std::string getTraderID();

    std::string getOrderID();

    void setMilli(long _mili);
    long getMilli();

    void setPrice(double price1);
    double getPrice();

    void setSize(int size1);
    int getSize();

    FIX::UtcTimeStamp gettime();

    void setOrderType(char ordertype1);
    char getOrderType();

    void setDestination(std::string destination1);
    std::string getDestination();
};
