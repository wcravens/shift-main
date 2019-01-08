#include "action.h"

action::action()
{
}

action::~action()
{
}

action::action(std::string stock1, double price1, int size1, std::string trader_id11, std::string trader_id21, char order_type11, char order_type21, std::string order_id11, std::string order_id21, char decision1, std::string destination1, FIX::UtcTimeStamp time0, FIX::UtcTimeStamp time11, FIX::UtcTimeStamp time21)
{
    stockname = stock1;
    price = price1;
    size = size1;
    trader_id1 = trader_id11;
    trader_id2 = trader_id21;
    order_type1 = order_type11;
    order_type2 = order_type21;
    order_id1 = order_id11;
    order_id2 = order_id21;
    decision = decision1;
    destination = destination1;
    exetime = time0;
    time1 = time11;
    time2 = time21;
}

action::action(const action& newaction)
{
    stockname = newaction.stockname;
    price = newaction.price;
    size = newaction.size;
    trader_id1 = newaction.trader_id1;
    trader_id2 = newaction.trader_id2;
    order_type1 = newaction.order_type1;
    order_type2 = newaction.order_type2;
    order_id1 = newaction.order_id1;
    order_id2 = newaction.order_id2;
    decision = newaction.decision;
    destination = newaction.destination;
    exetime = newaction.exetime;
    time1 = newaction.time1;
    time2 = newaction.time2;
}
