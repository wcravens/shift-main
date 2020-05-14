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
    Order(std::string symbol, double price, int size, Order::Type type, std::string destination, FIX::UtcTimeStamp simulationTime);
    Order(std::string symbol, std::string traderID, std::string orderID, double price, int size, Order::Type type, FIX::UtcTimeStamp simulationTime);

    auto operator==(const Order& other) -> bool;

    // getters
    auto getSymbol() const -> const std::string&;
    auto getTraderID() const -> const std::string&;
    auto getOrderID() const -> const std::string&;
    auto getMilli() const -> long;
    auto getPrice() const -> double;
    auto getSize() const -> int;
    auto getType() const -> Type;
    auto getTypeString() const -> std::string;
    auto getDestination() const -> const std::string&;
    auto getTime() const -> const FIX::UtcTimeStamp&;
    auto getAuctionCounter() const -> int;

    // setters
    void setSymbol(const std::string& symbol);
    void setMilli(long milli);
    void setPrice(double price);
    void setSize(int size);
    void setType(Type type);
    void setDestination(const std::string& destination);

    void incrementAuctionCounter();

private:
    std::string m_symbol;
    std::string m_traderID;
    std::string m_orderID;
    long m_milli;
    double m_price;
    int m_size;
    Type m_type;
    std::string m_destination;
    FIX::UtcTimeStamp m_simulationTime;
    int m_auctionCounter;
};
