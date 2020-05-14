#pragma once

#include "IMarketCreator.h"
#include "Market.h"
#include "Order.h"
#include "PriceLevel.h"

#include <deque>
#include <string>

namespace markets {

class FBAStockMarket : public Market {
public:
    FBAStockMarket(std::string symbol, double batchFrequencyS);
    FBAStockMarket(const market_creator_parameters_t& parameters);
    virtual ~FBAStockMarket() = default;

    auto getBatchFrequency() const -> double;
    void setBatchFrequency(double batchFrequencyS);

    auto getNextBatchAuction() const -> double;

    // function to start one matching engine, for market thread
    virtual void operator()() override;

    static inline void s_sortPriceLevel(PriceLevel& currentLevel)
    {
        currentLevel.sort([](const Order& a, const Order& b) {
            if (a.getAuctionCounter() == b.getAuctionCounter()) {
                return a.getSize() > b.getSize();
            }

            return a.getAuctionCounter() > b.getAuctionCounter();
        });
    };

    static auto s_determineExecutionSizes(int totalExecutionSize, const PriceLevel& currentLevel) -> std::deque<int>;

    void doBatchAuction();
    void doIncrementAuctionCounters();

    void doLocalCancelBid(Order& orderRef);
    void doLocalCancelAsk(Order& orderRef);

    void insertLocalBid(Order newBid);
    void insertLocalAsk(Order newAsk);

protected:
    double m_lastExecutionPrice;

    // FIXME: these variables should be in microseconds (TimeSetting update required)
    long m_batchFrequencyMS; // frequenty batch auctions batch frequency in milliseconds
    long m_nextBatchAuctionMS;
};

} // markets
