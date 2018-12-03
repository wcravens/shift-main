#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from Orderbook, which specifically used for Local Ask Orderbook.
 */
class CORECLIENT_EXPORTS OrderBookLocalAsk : public OrderBook {

public:
    OrderBookLocalAsk() = default;

    virtual void update(const shift::OrderBookEntry& entry) override;
};

} // shift
