#pragma once

#include "Newbook.h"
#include "Price.h"
#include "Quote.h"
#include "action.h"

#include <list>
#include <mutex>
#include <queue>
#include <string>

class Stock {

private:
    unsigned int depth = 5;
    typedef std::list<Price> ListPrice;
    typedef std::list<Quote> ListQuote;
    std::string name;

    std::list<Price> bid;
    std::list<Price> ask;

    std::list<Quote> globalbid;
    std::list<Quote> globalask;

    //buffer new quotes & trades received from database
    std::queue<Quote> new_global;
    //buffer new quotes received from clients
    std::queue<Quote> new_local;
    std::mutex new_global_mu;
    std::mutex new_local_mu;

    //list<action> actions;
    std::list<std::string> levels;
    std::list<Price>::iterator thisprice;
    std::list<Quote>::iterator thisquote;
    std::list<Quote>::iterator thisglobal;

public:
    //order book update queue
    std::list<Newbook> orderbookupdate;
    std::list<action> actions;

    Stock(const Stock& _stock)
    {
        name = _stock.name;
    }
    Stock(void);
    Stock(std::string name1);

    void setstockname(std::string name1);
    std::string getstockname();

    //buffer new quotes & trades received from database and clients
    void buf_new_global(Quote& quote);
    void buf_new_local(Quote& quote);
    //retrieve earlist quote buffered in new_global or new_local
    bool get_new_quote(Quote& quote);

    std::string levels_begin()
    {
        std::string findlevel = *levels.begin();
        return findlevel;
    }

    void levels_popfront()
    {
        levels.pop_front();
    }

    bool levels_empty()
    {
        if (levels.empty())
            return true;
        else
            return false;
    }

    void checkglobalbid(Quote& newquote, std::string type); //newquote to find execute in global orderbook
    void checkglobalask(Quote& newquote, std::string type); //newquote to find execute in global orderbook
    //void checklocal(ListPrice prices, globalprice newglobal);  // newglobal price to find execute in local orderbook!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void updateglobalbid(Quote newquote);
    void updateglobalask(Quote newquote);

    void execute(int size1, Quote& quote2, char decision1, std::string destination1);
    //decision, '2' means this is a trade record, 'C' means cancel record;
    void executeglobal(int size1, Quote& quote2, char decision1, std::string destination1);
    void dolimitbuy(Quote& newquote, std::string destination1);
    //destination, "Server" or thisquote.destination in global order book;
    void domarketbuy(Quote& newquote, std::string destination1);
    void docancelask(Quote& newquote, std::string destination1);
    void dolimitsell(Quote& newquote, std::string destination1);
    void domarketsell(Quote& newquote, std::string destination1);
    void docancelbid(Quote& newquote, std::string destination1);

    void bid_insert(Quote quote1); //{insert into bid list, sorted from big to small}
    void ask_insert(Quote quote1); //{insert into ask list, sorted from small to big};
    void setthisprice()
    {
        thisprice = ask.begin();
    }
    //void bid_delete(Price);
    //void ask_delete(Price);
    //bool findprice(double price, ListPrice prices); //
    void level(); // {subtract the size from the quotes and the underlying price, do the recording}

    void showglobal();

    void bookupdate(char _book, std::string _symbol, double _price, int _size, double _time);
    void bookupdate(char _book, std::string _symbol, double _price, int _size, double _time, std::string _destination);

    // hiukin
    // operation for local order
    void doLocalLimitBuy(Quote& newquote, std::string destination1);
    void doLocalLimitSell(Quote& newquote, std::string destination1);
    void doLocalMarketBuy(Quote& newquote, std::string destination1);
    void doLocalMarketSell(Quote& newquote, std::string destination1);

    ~Stock(void);

    std::string now_str();
};
