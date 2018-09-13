#pragma once

#include "CoreClient_EXPORTS.h"

#include <string>

#ifdef _WIN32
#include <crossguid/Guid.h>
#else
#include <shift/miscutils/crossguid/Guid.h>
#endif

namespace shift {

/**
 * @brief A class contains all information for a quote.
 */
class CORECLIENT_EXPORTS Order {

public:
    enum ORDER_TYPE : char {
        LIMIT_BUY = '1',
        LIMIT_SELL = '2',
        MARKET_BUY = '3',
        MARKET_SELL = '4',
        CANCEL_BID = '5',
        CANCEL_ASK = '6',
    };

    Order();
    Order(std::string symbol, double price, int size, ORDER_TYPE type);
    Order(std::string symbol, double price, int size, ORDER_TYPE type, std::string id);

    // Getters
    const std::string& getSymbol() const;
    double getPrice() const;
    int getSize() const;
    ORDER_TYPE getType() const;
    const std::string& getID() const;

    // Setters
    void setSymbol(std::string);
    void setPrice(double);
    void setSize(int);
    void setType(ORDER_TYPE);
    void setID(std::string);

private:
    std::string m_symbol; //!< Trading symbol for the current quote.
    double m_price;
    int m_size; //!< Trading size of the current quote, 1 size = 100 shares.
    ORDER_TYPE m_type; //!< '1':LimitBuy; '2':LimitSell; '3':MarketBuy; '4':MarketSell; '5':CancelBid; '6':CancelAsk
    std::string m_id; //!< Order ID of the current quote.
};

} // shift
