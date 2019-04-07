#pragma once

#include "ExecutionReport.h"
#include "OrderBookEntry.h"
#include "PriceLevel.h"
#include "Quote.h"

#include <list>
#include <mutex>
#include <queue>
#include <string>

#include <quickfix/FieldTypes.h>

class Stock {
public:
    std::list<ExecutionReport> executionReports;
    std::list<OrderBookEntry> orderBookUpdates;

    Stock() = default;
    Stock(const std::string& symbol);
    Stock(const Stock& other);

    const std::string& getSymbol() const;
    void setSymbol(const std::string& symbol);

    // buffer new quotes & trades received from database and clients
    void bufNewGlobal(const Quote& quote);
    void bufNewLocal(const Quote& quote);
    // retrieve earliest quote buffered in newGlobal or newLocal
    bool getNewQuote(Quote& quote);

    // decision '2' means this is a trade record, '4' means cancel record
    void execute(int size, Quote& quote, char decision, const std::string& destination);
    void executeGlobal(int size, Quote& quote, char decision, const std::string& destination);
    void doLimitBuy(Quote& newQuote, const std::string& destination);
    void doLimitSell(Quote& newQuote, const std::string& destination);
    void doMarketBuy(Quote& newQuote, const std::string& destination);
    void doMarketSell(Quote& newQuote, const std::string& destination);
    void doCancelBid(Quote& newQuote, const std::string& destination);
    void doCancelAsk(Quote& newQuote, const std::string& destination);

    // insert into bid list, sorted from highest to lowest
    void bidInsert(const Quote& quote);
    // insert into ask list, sorted from lowest to highest
    void askInsert(const Quote& quote);

    void bookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);
    void bookUpdate(OrderBookEntry::Type type, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);

    void showLocal();
    void showGlobal();

    void updateGlobalBid(const Quote& newQuote);
    void updateGlobalAsk(const Quote& newQuote);
    void checkGlobalBid(Quote& newQuote, const std::string& type);
    void checkGlobalAsk(Quote& newQuote, const std::string& type);

    // local order book operations
    void doLocalLimitBuy(Quote& newQuote, const std::string& destination);
    void doLocalLimitSell(Quote& newQuote, const std::string& destination);
    void doLocalMarketBuy(Quote& newQuote, const std::string& destination);
    void doLocalMarketSell(Quote& newQuote, const std::string& destination);

private:
    std::string m_symbol;
    unsigned int m_depth = 5;

    std::mutex m_mtxNewGlobal;
    std::mutex m_mtxNewLocal;

    // buffer new quotes & trades received from DE
    std::queue<Quote> m_newGlobal;
    // buffer new quotes received from clients
    std::queue<Quote> m_newLocal;

    std::list<Quote> m_globalBid;
    std::list<Quote> m_globalAsk;
    std::list<PriceLevel> m_localBid;
    std::list<PriceLevel> m_localAsk;

    std::list<Quote>::iterator m_thisGlobal;
    std::list<PriceLevel>::iterator m_thisPriceLevel;
    std::list<Quote>::iterator m_thisQuote;
};
