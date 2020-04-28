#pragma once

#include <string>

struct OrderConfirmation {
    std::string symbol;
    std::string traderID;
    std::string orderID;
    double price;
    int size;
    char orderType;

    OrderConfirmation(std::string symbol,
        std::string traderID,
        std::string orderID,
        double price,
        int size,
        char orderType)
        : symbol { std::move(symbol) }
        , traderID { std::move(traderID) }
        , orderID { std::move(orderID) }
        , price { price }
        , size { size }
        , orderType { orderType }
    {
    }
};
