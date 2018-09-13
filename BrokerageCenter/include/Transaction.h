#pragma once

#include <string>

struct Transaction {
    std::string traderID;
    std::string symbol;
    char orderDecision;
    std::string orderID;
    char orderType; // see Quote::ORDER_TYPE
    double execQty;
    double leftQty;
    double price;
    std::string serverTime;
    std::string transacTime;
    std::string execTime;
    std::string destination;
};
