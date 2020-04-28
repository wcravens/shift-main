#pragma once

#include <string>

/**
 *  @brief A class contains all information for an order.
 */
struct Order {
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

    enum Status : char { // see FIX::OrdStatus
        PENDING_NEW = 'A',
        NEW = '0',
        PARTIALLY_FILLED = '1',
        FILLED = '2',
        CANCELED = '4',
        PENDING_CANCEL = '6',
        REJECTED = '8',
    };

    Order() = default;
    Order(Order::Type type, std::string symbol, int size, double price, std::string id, std::string userID);

    static auto s_typeToString(Type type) -> std::string;

    Type getType() const;
    auto getSymbol() const -> const std::string&;
    auto getSize() const -> int;
    auto getExecutedSize() const -> int;
    auto getPrice() const -> double;
    auto getID() const -> const std::string&;
    auto getUserID() const -> const std::string&;
    auto getStatus() const -> Status;

    // Setters
    void setType(Type type);
    void setSymbol(const std::string& symbol);
    void setSize(int size);
    void setExecutedSize(int executedSize);
    void setPrice(double price);
    void setID(const std::string& id);
    void setUserID(const std::string& userID);
    void setStatus(Status status);

private:
    Type m_type;
    std::string m_symbol;
    int m_size;
    int m_executedSize;
    double m_price;
    std::string m_id;
    std::string m_userID;
    Status m_status;
};
