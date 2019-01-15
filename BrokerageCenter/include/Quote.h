#pragma once

#include <string>

/**
*   @brief The "Waiting List" item of the user's portfolio, maintains an instance of quote, and provides options-contract related controls.
*/
struct Quote {
    enum ORDER_TYPE : char {
        LIMIT_BUY = '1',
        LIMIT_SELL = '2',
        MARKET_BUY = '3',
        MARKET_SELL = '4',
        CANCEL_BID = '5',
        CANCEL_ASK = '6',
    };

    Quote();
    Quote(std::string symbol, std::string userName, std::string orderID, double price, int shareSize, ORDER_TYPE orderType); // for the server to receive

    const std::string& getSymbol() const;
    const std::string& getUserName() const;
    const std::string& getOrderID() const;
    void setPrice(double price);
    double getPrice() const;
    void setShareSize(int shareSize);
    int getShareSize() const;
    ORDER_TYPE getOrderType() const;

private:
    std::string m_symbol; ///<The stock name of quote
    std::string m_userName; ///<The user name of quote
    std::string m_orderID; ///<The order ID of quote
    double m_price; ///<The price of quote
    int m_shareSize; ///<The size of quote
    ORDER_TYPE m_orderType; ///<1:LimitBuy 2:LimitSell 3:MarketBuy 4:MarketSell 5:CancelBid 6:CancelAsk
};
