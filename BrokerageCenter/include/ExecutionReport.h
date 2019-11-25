#pragma once

#include "Order.h"

#include <string>

#include <quickfix/FieldTypes.h>

struct ExecutionReport {
    std::string userID;
    std::string orderID;
    Order::Type orderType;
    std::string orderSymbol;
    int currentSize;
    int executedSize;
    double orderPrice;
    Order::Status orderStatus;
    std::string destination;
    FIX::UtcTimeStamp simulationTime;
    FIX::UtcTimeStamp realTime;

    ExecutionReport(const std::string& userID,
        const std::string& orderID,
        Order::Type orderType,
        const std::string& orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        const std::string& destination,
        const FIX::UtcTimeStamp& simulationTime,
        const FIX::UtcTimeStamp& realTime)
        : userID(userID)
        , orderID(orderID)
        , orderType(orderType)
        , orderSymbol(orderSymbol)
        , currentSize(currentSize)
        , executedSize(executedSize)
        , orderPrice(orderPrice)
        , orderStatus(orderStatus)
        , destination(destination)
        , simulationTime(simulationTime)
        , realTime(realTime)
    {
    }

    ExecutionReport(const std::string& userID,
        const std::string& orderID,
        Order::Type orderType,
        const std::string& orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        const std::string& destination)
        : userID(userID)
        , orderID(orderID)
        , orderType(orderType)
        , orderSymbol(orderSymbol)
        , currentSize(currentSize)
        , executedSize(executedSize)
        , orderPrice(orderPrice)
        , orderStatus(orderStatus)
        , destination(destination)
        , simulationTime()
        , realTime()
    {
    }
};
