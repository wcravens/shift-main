#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct Transaction {
    std::string symbol;
    double price;
    int size;
    std::string destination;
    FIX::UtcTimeStamp simulationTime;
};
