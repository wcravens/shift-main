#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct Transaction {
    std::string symbol;
    int size;
    double price;
    std::string destination;
    FIX::UtcTimeStamp simulationTime;
};
