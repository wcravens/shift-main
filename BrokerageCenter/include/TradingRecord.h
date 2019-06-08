#pragma once

#include "Order.h"

#include "quickfix/FieldTypes.h"

#include <string>

/**
 * @brief Data structure for carrying Trading Record data between database and FIX components.
 *        Shall always use C++ standard types.
 */
struct TradingRecord {
    FIX::UtcTimeStamp realTime;
    FIX::UtcTimeStamp executionTime;
    std::string symbol; // or RIC
    double price;
    int size;
    std::string traderID1;
    std::string traderID2;
    std::string orderID1;
    std::string orderID2;
    Order::Type orderType1;
    Order::Type orderType2;
    char decision;
    std::string destination;
    FIX::UtcTimeStamp simulationTime1;
    FIX::UtcTimeStamp simulationTime2;

    TradingRecord(const FIX::UtcTimeStamp& realTime,
        const FIX::UtcTimeStamp& executionTime,
        const std::string& symbol,
        double price,
        int size,
        const std::string& traderID1,
        const std::string& traderID2,
        const std::string& orderID1,
        const std::string& orderID2,
        Order::Type orderType1,
        Order::Type orderType2,
        char decision,
        const std::string& destination,
        const FIX::UtcTimeStamp& simulationTime1,
        const FIX::UtcTimeStamp& simulationTime2)
        : realTime(realTime)
        , executionTime(executionTime)
        , symbol(symbol)
        , price(price)
        , size(size)
        , traderID1(traderID1)
        , traderID2(traderID2)
        , orderID1(orderID1)
        , orderID2(orderID2)
        , orderType1(orderType1)
        , orderType2(orderType2)
        , decision(decision)
        , destination(destination)
        , simulationTime1(simulationTime1)
        , simulationTime2(simulationTime2)
    {
    }
};
