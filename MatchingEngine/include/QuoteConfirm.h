#pragma once

#include <string>

struct QuoteConfirm {
    std::string clientID;
    std::string orderID;
    std::string symbol;
    double price;
    int size;
    char ordertype;
    std::string time;
};
