#pragma once

#include "Order.h"
#include "StockMarket.h"

#include <string>

namespace markets {

class ContinuousStockMarket : public StockMarket {
public:
    ContinuousStockMarket(std::string symbol);
    virtual ~ContinuousStockMarket() = default;

    // function to start one stock matching engine, for stock market thread
    virtual void operator()() override;

    void doGlobalLimitBuy(Order& orderRef);
    void doGlobalLimitSell(Order& orderRef);

    void updateGlobalBids(Order newBestBid);
    void updateGlobalAsks(Order newBestAsk);

    void doLocalLimitBuy(Order& orderRef);
    void doLocalLimitSell(Order& orderRef);
    void doLocalMarketBuy(Order& orderRef);
    void doLocalMarketSell(Order& orderRef);
    void doLocalCancelBid(Order& orderRef);
    void doLocalCancelAsk(Order& orderRef);

    void insertLocalBid(Order newBid);
    void insertLocalAsk(Order newAsk);
};

} // markets
