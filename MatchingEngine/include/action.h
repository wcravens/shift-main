#pragma once

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
    std::string exetime;
    std::string time1;
    std::string time2;
    char decision; //cancel or traded
    std::string destination;

    action(std::string stock1,
        double price1,
        int size1,
        std::string trader_id11,
        std::string trader_id21,
        char order_type11,
        char order_type21,
        std::string order_id11,
        std::string order_id21,
        std::string time0,
        std::string time11,
        std::string time21,
        char decision1,
        std::string destination1);
    action(action* newaction)
    {
        stockname = newaction->stockname;
        price = newaction->price;
        size = newaction->size;
        trader_id1 = newaction->trader_id1;
        trader_id2 = newaction->trader_id2;
        order_type1 = newaction->order_type1;
        order_type2 = newaction->order_type2;
        order_id1 = newaction->order_id1;
        order_id2 = newaction->order_id2;
        exetime = newaction->exetime;
        time1 = newaction->time1;
        time2 = newaction->time2;
        decision = newaction->decision;
        destination = newaction->destination;
    }
    action(const action& newaction)
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
        exetime = newaction.exetime;
        time1 = newaction.time1;
        time2 = newaction.time2;
        decision = newaction.decision;
        destination = newaction.destination;
    }
    void show()
    {
        //cout<<stockname<<"   "<<price<<"   "<<size<<"   "<<order_id1 <<"   "<<order_id2 <<"   "<< exetime<<"\n";
    }
    action(void);
    ~action(void);
};
