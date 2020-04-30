#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from Orderbook, which specifically used for Global Bid Order Book.
 */
class CORECLIENT_EXPORTS OrderBookGlobalBid : public OrderBook {
public:
    OrderBookGlobalBid(std::string symbol);

    virtual void update(shift::OrderBookEntry&& entry) override;
};

} // shift
