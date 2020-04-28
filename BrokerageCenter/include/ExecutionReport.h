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

    ExecutionReport(std::string userID,
        std::string orderID,
        Order::Type orderType,
        std::string orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        std::string destination,
        FIX::UtcTimeStamp simulationTime,
        FIX::UtcTimeStamp realTime)
        : userID { std::move(userID) }
        , orderID { std::move(orderID) }
        , orderType { orderType }
        , orderSymbol { std::move(orderSymbol) }
        , currentSize { currentSize }
        , executedSize { executedSize }
        , orderPrice { orderPrice }
        , orderStatus { orderStatus }
        , destination { std::move(destination) }
        , simulationTime { std::move(simulationTime) }
        , realTime { std::move(realTime) }
    {
    }

    ExecutionReport(std::string userID,
        std::string orderID,
        Order::Type orderType,
        std::string orderSymbol,
        int currentSize,
        int executedSize,
        double orderPrice,
        Order::Status orderStatus,
        std::string destination)
        : userID { std::move(userID) }
        , orderID { std::move(orderID) }
        , orderType { orderType }
        , orderSymbol { std::move(orderSymbol) }
        , currentSize { currentSize }
        , executedSize { executedSize }
        , orderPrice { orderPrice }
        , orderStatus { orderStatus }
        , destination { std::move(destination) }
        , simulationTime {}
        , realTime {}
    {
    }
};
