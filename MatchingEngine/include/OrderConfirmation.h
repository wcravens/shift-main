#pragma once

#include <string>

struct OrderConfirmation {
    std::string symbol;
    std::string traderID;
    std::string orderID;
    double price;
    int size;
    char orderType;

    OrderConfirmation(const std::string& symbol,
        const std::string& traderID,
        const std::string& orderID,
        double price,
        int size,
        char orderType)
        : symbol(symbol)
        , traderID(traderID)
        , orderID(orderID)
        , price(price)
        , size(size)
        , orderType(orderType)
    {
    }
};
