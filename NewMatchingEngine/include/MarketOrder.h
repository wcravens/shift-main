#pragma once

#include "Order.h"

#include <memory>
#include <string>

#include <quickfix/FieldTypes.h>

class MarketOrder : public Order {

protected:
    MarketOrder* cloneImpl() const override; // clone pattern

public:
    MarketOrder(const MarketOrder&) = default; // copy constructor
    MarketOrder& operator=(const MarketOrder&) & = default; // copy assignment
    MarketOrder(MarketOrder&&) = default; // move constructor
    MarketOrder& operator=(MarketOrder&&) & = default; // move assignment
    virtual ~MarketOrder() = default; // virtual destructor

    MarketOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time);

    Order::TYPE getType() const override;
};
