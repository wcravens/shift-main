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

    enum Status : char { // see FIX::OrdStatus
        PENDING_NEW = 'A',
        NEW = '0',
        PARTIALLY_FILLED = '1',
        FILLED = '2',
        CANCELED = '4',
        PENDING_CANCEL = '6',
        REJECTED = '8',
    };

    static auto s_roundNearest(double value, double nearest) -> double;

    Order() = default;
    Order(Type type, std::string symbol, int size, double price = 0.0, std::string id = "");

    // Getters
    auto getType() const -> Type;
    auto getTypeString() const -> std::string;
    auto getSymbol() const -> const std::string&;
    auto getSize() const -> int;
    auto getExecutedSize() const -> int;
    auto getPrice() const -> double;
    auto getExecutedPrice() const -> double;
    auto getID() const -> const std::string&;
    auto getStatus() const -> Status;
    auto getStatusString() const -> std::string;
    auto getTimestamp() const -> const std::chrono::system_clock::time_point&;

    // Setters
    void setType(Type type);
    void setSymbol(const std::string& symbol);
    void setSize(int size);
    void setExecutedSize(int executedSize);
    void setPrice(double price);
    void setExecutedPrice(double executedPrice);
    void setID(const std::string& id);
    void setStatus(Status status);
    void setTimestamp();

private:
    Type m_type;
    std::string m_symbol;
    int m_size;
    int m_executedSize;
    double m_price;
    double m_executedPrice;
    std::string m_id;
    Status m_status;
    std::chrono::system_clock::time_point m_timestamp;
};

} // shift
