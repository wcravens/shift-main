#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct QuoteConfirm {
    std::string clientID;
    std::string orderID;
    std::string symbol;
    double price;
    int size;
    char orderType;
    FIX::UtcTimeStamp time;
};
