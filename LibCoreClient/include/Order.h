#pragma once

#include "CoreClient_EXPORTS.h"

#include <chrono>
#include <string>

#ifdef _WIN32
#include <crossguid/Guid.h>
#else
#include <shift/miscutils/crossguid/Guid.h>
#endif

namespace shift {

/**
 * @brief A class contains all information for an order.
 */
class CORECLIENT_EXPORTS Order {

public:
    enum Type : char {
        LIMIT_BUY = '1',
        LIMIT_SELL = '2',
        MARKET_BUY = '3',
        MARKET_SELL = '4',
        CANCEL_BID = '5',
        CANCEL_ASK = '6',
    };

    static double s_decimalTruncate(double value, int precision);

    Order();
    Order(Type type, const std::string& symbol, int size, double price = 0.0, const std::string& id = "");

    // Getters
    Type getType() const;
    const std::string& getSymbol() const;
    int getSize() const;
    double getPrice() const;
    const std::string& getID() const;
    const std::chrono::system_clock::time_point& getTimestamp() const;

    // Setters
    void setType(Type type);
    void setSymbol(const std::string& symbol);
    void setSize(int size);
    void setPrice(double price);
    void setID(const std::string& id);
    void setTimestamp();

private:
    Type m_type;
    std::string m_symbol;
    int m_size;
    double m_price;
    std::string m_id;
    std::chrono::system_clock::time_point m_timestamp;
};

} // shift
