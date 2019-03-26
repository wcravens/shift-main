#pragma once

#include <quickfix/Application.h>

#include <string>

struct action {
    std::string stockname;
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
    FIX::UtcTimeStamp exetime;
    FIX::UtcTimeStamp time1;
    FIX::UtcTimeStamp time2;

    action(){};

    ~action(){};

    action(std::string _stockname,
        double _price,
        int _size,
        std::string _traderID1,
        std::string _traderID2,
        char _orderType1,
        char _orderType2,
        std::string _orderID1,
        std::string _orderID2,
        char _decision,
        std::string _destination,
        FIX::UtcTimeStamp _exetime,
        FIX::UtcTimeStamp _time1,
        FIX::UtcTimeStamp _time2)
    {
        stockname = _stockname;
        price = _price;
        size = _size;
        traderID1 = _traderID1;
        traderID2 = _traderID2;
        orderType1 = _orderType1;
        orderType2 = _orderType2;
        orderID1 = _orderID1;
        orderID2 = _orderID2;
        decision = _decision;
        destination = _destination;
        exetime = _exetime;
        time1 = _time1;
        time2 = _time2;
    }

    action(const action& _act)
    {
        stockname = _act.stockname;
        price = _act.price;
        size = _act.size;
        traderID1 = _act.traderID1;
        traderID2 = _act.traderID2;
        orderType1 = _act.orderType1;
        orderType2 = _act.orderType2;
        orderID1 = _act.orderID1;
        orderID2 = _act.orderID2;
        decision = _act.decision;
        destination = _act.destination;
        exetime = _act.exetime;
        time1 = _act.time1;
        time2 = _act.time2;
    }
};
