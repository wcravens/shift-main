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

    ExecutionReport(std::string symbol,
        double price,
        int size,
        std::string traderID1,
        std::string traderID2,
        Order::Type orderType1,
        Order::Type orderType2,
        std::string orderID1,
        std::string orderID2,
        char decision,
        std::string destination,
        FIX::UtcTimeStamp simulationTime1,
        FIX::UtcTimeStamp simulationTime2)
        : symbol { std::move(symbol) }
        , price { price }
        , size { size }
        , traderID1 { std::move(traderID1) }
        , traderID2 { std::move(traderID2) }
        , orderType1 { orderType1 }
        , orderType2 { orderType2 }
        , orderID1 { std::move(orderID1) }
        , orderID2 { std::move(orderID2) }
        , decision { decision }
        , destination { std::move(destination) }
        , simulationTime1 { std::move(simulationTime1) }
        , simulationTime2 { std::move(simulationTime2) }
    {
    }
};
