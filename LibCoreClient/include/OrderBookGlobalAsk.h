#pragma once

#include "CoreClient_EXPORTS.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"

namespace shift {

/**
 * @brief A child class from OrderBook, which specifically used for GlobalAsk OrderBook.
 */
class CORECLIENT_EXPORTS OrderBookGlobalAsk : public OrderBook {

public:
    OrderBookGlobalAsk();

    virtual void update(shift::OrderBookEntry entry) override;
};

} // shift
