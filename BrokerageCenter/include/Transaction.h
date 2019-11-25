#pragma once

#include <string>

#include <quickfix/FieldTypes.h>

struct Transaction {
    std::string symbol;
    int size;
    double price;
    std::string destination;
    FIX::UtcTimeStamp simulationTime;

    Transaction(const std::string& symbol,
        int size,
        double price,
        const std::string& destination,
        const FIX::UtcTimeStamp& simulationTime)
        : symbol(symbol)
        , size(size)
        , price(price)
        , destination(destination)
        , simulationTime(simulationTime)
    {
    }
};
