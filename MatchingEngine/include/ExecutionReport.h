#pragma once

#include "Order.h"

#include <string>

#include <quickfix/FieldTypes.h>

struct ExecutionReport {
    std::string symbol;
    double price;
    int size;
    std::string traderID1;
    std::string traderID2;
    Order::Type orderType1;
    Order::Type orderType2;
    std::string orderID1;
    std::string orderID2;
    char decision; // trade ('2') or cancel ('4')
    std::string destination;
    FIX::UtcTimeStamp simulationTime1;
    FIX::UtcTimeStamp simulationTime2;

    ExecutionReport(const std::string& symbol,
        double price,
        int size,
        const std::string& traderID1,
        const std::string& traderID2,
        Order::Type orderType1,
        Order::Type orderType2,
        const std::string& orderID1,
        const std::string& orderID2,
        char decision,
        const std::string& destination,
        const FIX::UtcTimeStamp& simulationTime1,
        const FIX::UtcTimeStamp& simulationTime2)
        : symbol(symbol)
        , price(price)
        , size(size)
        , traderID1(traderID1)
        , traderID2(traderID2)
        , orderType1(orderType1)
        , orderType2(orderType2)
        , orderID1(orderID1)
        , orderID2(orderID2)
        , decision(decision)
        , destination(destination)
        , simulationTime1(simulationTime1)
        , simulationTime2(simulationTime2)
    {
    }
};
