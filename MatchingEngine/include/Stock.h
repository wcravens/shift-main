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

    // buffer new quotes & trades received from database
    std::queue<Quote> m_newGlobal;
    // buffer new quotes received from clients
    std::queue<Quote> m_newLocal;

    std::mutex m_newGlobal_mu;
    std::mutex m_newLocal_mu;

    std::list<std::string> m_levels;
    std::list<Price>::iterator m_thisPrice;
    std::list<Quote>::iterator m_thisQuote;
    std::list<Quote>::iterator m_thisGlobal;

public:
    // order book update queue
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

    // buffer new quotes & trades received from database and clients
    void bufNewGlobal(Quote& quote);
    void bufNewLocal(Quote& quote);
    // retrieve earliest quote buffered in newGlobal or newLocal
    bool getNewQuote(Quote& quote);

    void checkGlobalBid(Quote& newQuote, std::string type);
    void checkGlobalAsk(Quote& newQuote, std::string type);
    void updateGlobalBid(Quote newQuote);
    void updateGlobalAsk(Quote newQuote);

    // decision '2' means this is a trade record, '4' means cancel record
    void execute(int size, Quote& quote, char decision, std::string destination);
    void executeGlobal(int size, Quote& quote, char decision, std::string destination);
    void doLimitBuy(Quote& newQuote, std::string destination);
    void doMarketBuy(Quote& newQuote, std::string destination);
    void doCancelAsk(Quote& newQuote, std::string destination);
    void doLimitSell(Quote& newQuote, std::string destination);
    void doMarketSell(Quote& newQuote, std::string destination);
    void doCancelBid(Quote& newQuote, std::string destination);

    // insert into bid list, sorted from highest to lowest
    void bidInsert(Quote quote);
    // insert into ask list, sorted from lowest to highest
    void askInsert(Quote quote);

    void showLocal();
    void showGlobal();

    void bookUpdate(char book, std::string symbol, double price, int size, FIX::UtcTimeStamp time);
    void bookUpdate(char book, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp time);

    // local order book operations
    void doLocalLimitBuy(Quote& newQuote, std::string destination);
    void doLocalLimitSell(Quote& newQuote, std::string destination);
    void doLocalMarketBuy(Quote& newQuote, std::string destination);
    void doLocalMarketSell(Quote& newQuote, std::string destination);

    std::string now_str();
};
