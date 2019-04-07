#pragma once

#include <quickfix/Application.h>

#include <string>

struct ExecutionReport {
    std::string symbol;
    double price;
    int size;
    std::string traderID1;
    std::string traderID2;
    char orderType1;
    char orderType2;
    std::string orderID1;
    std::string orderID2;
    char decision; // trade ('2') or cancel ('4')
    std::string destination;
    FIX::UtcTimeStamp execTime;
    FIX::UtcTimeStamp time1;
    FIX::UtcTimeStamp time2;

    ExecutionReport(const std::string& symbol,
        double price,
        int size,
        const std::string& traderID1,
        const std::string& traderID2,
        char orderType1,
        char orderType2,
        const std::string& orderID1,
        const std::string& orderID2,
        char decision,
        const std::string& destination,
        const FIX::UtcTimeStamp& execTime,
        const FIX::UtcTimeStamp& time1,
        const FIX::UtcTimeStamp& time2)
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
        , execTime(execTime)
        , time1(time1)
        , time2(time2)
    {
    }
};
