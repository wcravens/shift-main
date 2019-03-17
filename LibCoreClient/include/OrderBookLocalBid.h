#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from Orderbook, which specifically used for Local Bid Orderbook.
 */
class CORECLIENT_EXPORTS OrderBookLocalBid : public OrderBook {

public:
    OrderBookLocalBid(const std::string& symbol);

    virtual void update(shift::OrderBookEntry&& entry) override;
};

} // shift
