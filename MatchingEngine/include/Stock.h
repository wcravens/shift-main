#pragma once

#include "Action.h"
#include "Newbook.h"
#include "Price.h"
#include "Quote.h"

#include <list>
#include <mutex>
#include <queue>
#include <string>

#include <quickfix/FieldTypes.h>

class Stock {

private:
    typedef std::list<Price> ListPrice;
    typedef std::list<Quote> ListQuote;

    unsigned int m_depth = 5;

    std::string m_name;

    std::list<Price> m_bid;
    std::list<Price> m_ask;

    std::list<Quote> m_globalBid;
    std::list<Quote> m_globalAsk;

    //buffer new quotes & trades received from database
    std::queue<Quote> m_newGlobal;
    //buffer new quotes received from clients
    std::queue<Quote> m_newLocal;

    std::mutex m_newGlobal_mu;
    std::mutex m_newLocal_mu;

    //list<action> actions;
    std::list<std::string> m_levels;
    std::list<Price>::iterator m_thisPrice;
    std::list<Quote>::iterator m_thisQuote;
    std::list<Quote>::iterator m_thisGlobal;

public:
    //order book update queue
    std::list<Newbook> orderbookupdate;
    std::list<Action> actions;

    Stock();
    Stock(const Stock& stock);
    Stock(std::string name);
    ~Stock();

    void setStockname(std::string name);
    std::string getStockname();

    void setThisPrice()
    {
        m_thisPrice = m_ask.begin();
    }

    //buffer new quotes & trades received from database and clients
    void bufNewGlobal(Quote& quote);
    void bufNewLocal(Quote& quote);
    //retrieve earlist quote buffered in new_global or new_local
    bool getNewQuote(Quote& quote);

    void checkGlobalBid(Quote& newquote, std::string type); //newquote to find execute in global orderbook
    void checkGlobalAsk(Quote& newquote, std::string type); //newquote to find execute in global orderbook
    //void checklocal(ListPrice prices, globalprice newglobal);  // newglobal price to find execute in local orderbook!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void updateGlobalBid(Quote newquote);
    void updateGlobalAsk(Quote newquote);

    void execute(int size, Quote& quote, char decision, std::string destination);
    //decision, '2' means this is a trade record, '4' means cancel record;
    void executeGlobal(int size, Quote& quote, char decision, std::string destination);
    void doLimitBuy(Quote& newquote, std::string destination);
    //destination, "SHIFT" or thisquote.destination in global order book;
    void doMarketBuy(Quote& newquote, std::string destination);
    void doCancelAsk(Quote& newquote, std::string destination);
    void doLimitSell(Quote& newquote, std::string destination);
    void doMarketSell(Quote& newquote, std::string destination);
    void doCancelBid(Quote& newquote, std::string destination);

    void bidInsert(Quote quote); //{insert into bid list, sorted from big to small}
    void askInsert(Quote quote); //{insert into ask list, sorted from small to big};

    //void bid_delete(Price);
    //void ask_delete(Price);
    //bool findprice(double price, ListPrice prices); //
    void level(); // {subtract the size from the quotes and the underlying price, do the recording}

    void showGlobal();

    void bookUpdate(char book, std::string symbol, double price, int size, FIX::UtcTimeStamp utctime);
    void bookUpdate(char book, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp utctime);

    // hiukin
    // operation for local order
    void doLocalLimitBuy(Quote& newquote, std::string destination);
    void doLocalLimitSell(Quote& newquote, std::string destination);
    void doLocalMarketBuy(Quote& newquote, std::string destination);
    void doLocalMarketSell(Quote& newquote, std::string destination);

    std::string now_str();
};
