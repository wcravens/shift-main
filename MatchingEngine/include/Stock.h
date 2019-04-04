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

    std::string m_stockName;

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
    std::list<Newbook> orderBookUpdate;
    std::list<Action> actions;

    Stock();
    Stock(const Stock& other);
    Stock(std::string name);
    ~Stock();

    void setStockName(std::string stockName);
    std::string getStockName();

    void setThisPrice()
    {
        m_thisPrice = m_ask.begin();
    }

    //buffer new quotes & trades received from database and clients
    void bufNewGlobal(Quote& quote);
    void bufNewLocal(Quote& quote);
    //retrieve earlist quote buffered in new_global or new_local
    bool getNewQuote(Quote& quote);

    void checkGlobalBid(Quote& newQuote, std::string type); //newQuote to find execute in global orderbook
    void checkGlobalAsk(Quote& newQuote, std::string type); //newQuote to find execute in global orderbook
    //void checklocal(ListPrice prices, globalprice newglobal);  // newglobal price to find execute in local orderbook!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void updateGlobalBid(Quote newQuote);
    void updateGlobalAsk(Quote newQuote);

    void execute(int size, Quote& quote, char decision, std::string destination);
    //decision, '2' means this is a trade record, '4' means cancel record;
    void executeGlobal(int size, Quote& quote, char decision, std::string destination);
    void doLimitBuy(Quote& newQuote, std::string destination);
    //destination, "SHIFT" or thisquote.destination in global order book;
    void doMarketBuy(Quote& newQuote, std::string destination);
    void doCancelAsk(Quote& newQuote, std::string destination);
    void doLimitSell(Quote& newQuote, std::string destination);
    void doMarketSell(Quote& newQuote, std::string destination);
    void doCancelBid(Quote& newQuote, std::string destination);

    void bidInsert(Quote quote); //{insert into bid list, sorted from big to small}
    void askInsert(Quote quote); //{insert into ask list, sorted from small to big};

    //void bid_delete(Price);
    //void ask_delete(Price);
    //bool findprice(double price, ListPrice prices); //
    void level(); // {subtract the size from the quotes and the underlying price, do the recording}

    void showGlobal();

    void bookUpdate(char book, std::string symbol, double price, int size, FIX::UtcTimeStamp time);
    void bookUpdate(char book, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp time);

    // hiukin
    // operation for local order
    void doLocalLimitBuy(Quote& newQuote, std::string destination);
    void doLocalLimitSell(Quote& newQuote, std::string destination);
    void doLocalMarketBuy(Quote& newQuote, std::string destination);
    void doLocalMarketSell(Quote& newQuote, std::string destination);

    std::string now_str();
};
