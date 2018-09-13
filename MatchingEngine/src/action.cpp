#include "action.h"

action::action(std::string stock1, double price1, int size1, std::string trader_id11, std::string trader_id21, char order_type11, char order_type21, std::string order_id11, std::string order_id21, std::string time0, std::string time11, std::string time21, char decision1, std::string destination1)
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
    exetime = time0;
    time1 = time11;
    time2 = time21;
    decision = decision1;
    destination = destination1;
}

action::action(void)
{
}

action::~action(void)
{
}
