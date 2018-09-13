#pragma once

#include <string>

struct Report {
    std::string clientID;
    std::string orderID; //server_order_id
    char status; //order status: http://fixwiki.org/fixwiki/OrdStatus
    std::string execTime;
    std::string serverTime; //equles to Text
    std::string symbol;
    char orderType; // see Quote::ORDER_TYPE
    double price;
    double shareSize;
};
