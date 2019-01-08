#pragma once

#include <quickfix/Application.h>

#include <string>

class action {
public:
    std::string stockname;
    double price;
    int size;
    std::string trader_id1;
    std::string trader_id2;
    char order_type1;
    char order_type2;
    std::string order_id1;
    std::string order_id2;
    char decision; //cancel or traded
    std::string destination;
    FIX::UtcTimeStamp exetime;
    FIX::UtcTimeStamp time1;
    FIX::UtcTimeStamp time2;

    action();

    ~action();

    action(std::string stock1,
        double price1,
        int size1,
        std::string trader_id11,
        std::string trader_id21,
        char order_type11,
        char order_type21,
        std::string order_id11,
        std::string order_id21,
        char decision1,
        std::string destination1,
        FIX::UtcTimeStamp time0,
        FIX::UtcTimeStamp time11,
        FIX::UtcTimeStamp time21);

    action(const action& newaction);
};
