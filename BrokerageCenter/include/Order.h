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
    };

    enum Status : char { // See FIX::OrdStatus
        PENDING_NEW = 'A',
        NEW = '0',
        PARTIALLY_FILLED = '1',
        FILLED = '2',
        CANCELED = '4',
        PENDING_CANCEL = '6',
        REJECTED = '8',
    };

    Order() = default;
    Order(Type type, const std::string& symbol, int size, double price, const std::string& id, const std::string& userID);

    Type getType() const;
    static const char* s_typeToString(Type type);
    const std::string& getSymbol() const;
    int getSize() const;
    int getExecutedSize() const;
    double getPrice() const;
    const std::string& getID() const;
    const std::string& getUserID() const;
    Status getStatus() const;

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
