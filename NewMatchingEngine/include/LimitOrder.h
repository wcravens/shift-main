#pragma once

#include "Order.h"

#include <memory>
#include <string>

#include <quickfix/FieldTypes.h>

class LimitOrder : public Order {

    ///////////////////////////////////////////////////////////////////////////////
public:
    enum class TIME_IN_FORCE : char {
        DAY = '0', // Day (or session)
        GTC = '1', // Good Till Cancel (GTC)
        // OPG = '2', // At the Opening (OPG)
        IOC = '3', // Immediate Or Cancel (IOC)
        FOK = '4', // Fill Or Kill (FOK)
        // GTX = '5', // Good Till Crossing (GTX)
        // GTD = '6', // Good Till Date (GTD)
    };
    ///////////////////////////////////////////////////////////////////////////////

protected:
    double m_price;
    double m_displayQuantity;
    LimitOrder::TIME_IN_FORCE m_timeInForce;

protected:
    virtual auto cloneImpl() const -> LimitOrder* override; // clone pattern

public:
    LimitOrder(const LimitOrder&) = default; // copy constructor
    auto operator=(const LimitOrder&) & -> LimitOrder& = default; // lvalue-only copy assignment
    LimitOrder(LimitOrder&&) = default; // move constructor
    auto operator=(LimitOrder&&) & -> LimitOrder& = default; // lvalue-only move assignment
    virtual ~LimitOrder() = default; // virtual destructor

    // Limit Order
    LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time, double price);

    // Limit Order with Hidden Volume
    LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time, double price, double displayQuantity);

    // Limit Order with Time In Force
    LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time, double price, LimitOrder::TIME_IN_FORCE timeInForce);

    // Limit Order with Hidden Volume & Time In Force
    LimitOrder(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time, double price, double displayQuantity, LimitOrder::TIME_IN_FORCE timeInForce);

    virtual auto getType() const -> Order::TYPE override;

    auto getPrice() const -> double;
    auto getDisplayQuantity() const -> double;
    auto getTimeInForce() const -> LimitOrder::TIME_IN_FORCE;
};
