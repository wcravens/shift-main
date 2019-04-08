#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

class Order {
public:
    enum Type : char {
        LIMIT_BUY = '1',
        LIMIT_SELL = '2',
        MARKET_BUY = '3',
        MARKET_SELL = '4',
        CANCEL_BID = '5',
        CANCEL_ASK = '6',
        TRTH_TRADE = '7',
        TRTH_BID = '8',
        TRTH_ASK = '9',
    };

    Order() = default;
    Order(const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);
    Order(const std::string& symbol, const std::string& traderID, const std::string& orderID, double price, int size, Type type, const FIX::UtcTimeStamp& time);

    // Getters
    const std::string& getSymbol() const;
    const std::string& getTraderID() const;
    const std::string& getOrderID() const;
    long getMilli() const;
    double getPrice() const;
    int getSize() const;
    Type getType() const;
    const std::string& getDestination() const;
    const FIX::UtcTimeStamp& getTime() const;

    // Setters
    void setSymbol(const std::string& symbol);
    void setMilli(long milli);
    void setPrice(double price);
    void setSize(int size);
    void setType(Type type);
    void setDestination(const std::string& destination);

private:
    std::string m_symbol;
    std::string m_traderID;
    std::string m_orderID;
    long m_milli;
    double m_price;
    int m_size;
    Type m_type;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
