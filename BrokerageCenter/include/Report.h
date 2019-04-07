#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

struct Report {
    std::string username;
    std::string orderID;
    Order::Type orderType;
    std::string orderSymbol;
    int currentSize;
    int executedSize;
    double orderPrice;
    Order::Status orderStatus;
    std::string destination;
    FIX::UtcTimeStamp execTime;
    FIX::UtcTimeStamp serverTime;

    Report(const std::string& username,
        const std::string& orderID,
        Order::Type orderType,
        const std::string& orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        const std::string& destination,
        const FIX::UtcTimeStamp& execTime,
        const FIX::UtcTimeStamp& serverTime)
        : username(username)
        , orderID(orderID)
        , orderType(orderType)
        , orderSymbol(orderSymbol)
        , currentSize(currentSize)
        , executedSize(executedSize)
        , orderPrice(orderPrice)
        , orderStatus(orderStatus)
        , destination(destination)
        , execTime(execTime)
        , serverTime(serverTime)
    {
    }

    Report(const std::string& username,
        const std::string& orderID,
        Order::Type orderType,
        const std::string& orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        const std::string& destination)
        : username(username)
        , orderID(orderID)
        , orderType(orderType)
        , orderSymbol(orderSymbol)
        , currentSize(currentSize)
        , executedSize(executedSize)
        , orderPrice(orderPrice)
        , orderStatus(orderStatus)
        , destination(destination)
        , execTime()
        , serverTime()
    {
    }
};
