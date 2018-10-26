#pragma once

#include <quickfix/Application.h>

#include <string>

struct QuoteConfirm
{
    std::string clientID;
    std::string orderID;
    std::string symbol;
    double price;
    int size;
    char ordertype;
    FIX::UtcTimeStamp time;
};
