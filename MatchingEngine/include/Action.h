#pragma once

#include <quickfix/Application.h>

#include <string>

struct Action {
    std::string stockName;
    double price;
    int size;
    std::string traderID1;
    std::string traderID2;
    char orderType1;
    char orderType2;
    std::string orderID1;
    std::string orderID2;
    char decision; //cancel or traded
    std::string destination;
    FIX::UtcTimeStamp execTime;
    FIX::UtcTimeStamp time1;
    FIX::UtcTimeStamp time2;

    Action(){};

    ~Action(){};

    Action(std::string stockName,
        double price,
        int size,
        std::string traderID1,
        std::string traderID2,
        char orderType1,
        char orderType2,
        std::string orderID1,
        std::string orderID2,
        char decision,
        std::string destination,
        FIX::UtcTimeStamp execTime,
        FIX::UtcTimeStamp time1,
        FIX::UtcTimeStamp time2)
        : stockName(stockName)
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

    Action(const Action& other)
        : stockName(other.stockName)
        , price(other.price)
        , size(other.size)
        , traderID1(other.traderID1)
        , traderID2(other.traderID2)
        , orderType1(other.orderType1)
        , orderType2(other.orderType2)
        , orderID1(other.orderID1)
        , orderID2(other.orderID2)
        , decision(other.decision)
        , destination(other.destination)
        , execTime(other.execTime)
        , time1(other.time1)
        , time2(other.time2)
    {
    }
};
