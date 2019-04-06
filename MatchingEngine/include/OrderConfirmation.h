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
};
