#pragma once

#include "IMarketCreator.h"
#include "Market.h"
#include "Order.h"

#include <string>

namespace markets {

class ContinuousStockMarket : public Market {
public:
    ContinuousStockMarket(std::string symbol);
    ContinuousStockMarket(const market_creator_parameters_t& parameters);
    virtual ~ContinuousStockMarket() = default;

    // function to start one matching engine, for market thread
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
