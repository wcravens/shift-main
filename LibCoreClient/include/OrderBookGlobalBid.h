#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from Orderbook, which specifically used for Global Bid Orderbook.
 */
class CORECLIENT_EXPORTS OrderBookGlobalBid : public OrderBook {

public:
    OrderBookGlobalBid();

    virtual void update(const shift::OrderBookEntry& entry) override;
};

} // shift
