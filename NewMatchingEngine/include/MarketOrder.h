#pragma once

#include "Order.h"

#include <memory>
#include <string>

#include <quickfix/FieldTypes.h>

class MarketOrder : public Order {

protected:
    virtual auto cloneImpl() const -> MarketOrder* override; // clone pattern

public:
    MarketOrder(const MarketOrder&) = default; // copy constructor
    auto operator=(const MarketOrder&) & -> MarketOrder& = default; // lvalue-only copy assignment
    MarketOrder(MarketOrder&&) = default; // move constructor
    auto operator=(MarketOrder&&) & -> MarketOrder& = default; // lvalue-only move assignment
    virtual ~MarketOrder() = default; // virtual destructor

    MarketOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time);

    virtual auto getType() const -> Order::TYPE override;
};
