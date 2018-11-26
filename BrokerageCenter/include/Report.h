#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct Report {
    std::string userName;
    std::string orderID; //server_order_id
    char status; //order status: http://fixwiki.org/fixwiki/OrdStatus
    std::string symbol;
    char orderType; // see Quote::ORDER_TYPE
    double price;
    double shareSize;
    FIX::UtcTimeStamp execTime;
    FIX::UtcTimeStamp serverTime;
};
