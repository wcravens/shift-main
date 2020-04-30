#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from Orderbook, which specifically used for Local Ask Order Book.
 */
class CORECLIENT_EXPORTS OrderBookLocalAsk : public OrderBook {
public:
    OrderBookLocalAsk(std::string symbol);

    virtual void update(shift::OrderBookEntry&& entry) override;
};

} // shift
