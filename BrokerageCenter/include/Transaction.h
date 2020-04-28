#pragma once

#include <string>

#include <quickfix/FieldTypes.h>

struct Transaction {
    std::string symbol;
    int size;
    double price;
    std::string destination;
    FIX::UtcTimeStamp simulationTime;

    Transaction(std::string symbol,
        int size,
        double price,
        std::string destination,
        FIX::UtcTimeStamp simulationTime)
        : symbol { std::move(symbol) }
        , size { size }
        , price { price }
        , destination { std::move(destination) }
        , simulationTime { std::move(simulationTime) }
    {
    }
};
