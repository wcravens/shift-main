#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct OrderConfirmation {
    std::string symbol;
    std::string traderID;
    std::string orderID;
    double price;
    int size;
    char orderType;
    FIX::UtcTimeStamp time;

    OrderConfirmation(const std::string& symbol,
        const std::string& traderID,
        const std::string& orderID,
        double price,
        int size,
        char orderType,
        const FIX::UtcTimeStamp& time)
        : symbol(symbol)
        , traderID(traderID)
        , orderID(orderID)
        , price(price)
        , size(size)
        , orderType(orderType)
        , time(time)
    {
    }
};
