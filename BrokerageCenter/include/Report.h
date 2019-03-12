#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct Report {
    std::string username;
    std::string orderID;
    Order::Type orderType;
    std::string orderSymbol;
    int orderSize;
    double orderPrice;
    char orderStatus;
    std::string destination;
    FIX::UtcTimeStamp execTime;
    FIX::UtcTimeStamp serverTime;
};
