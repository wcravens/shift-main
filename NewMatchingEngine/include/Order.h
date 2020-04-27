#pragma once

#include "Instrument.h"

#include <memory>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

class Order {

    ///////////////////////////////////////////////////////////////////////////////
public:
    enum class TYPE : char {
        MARKET = '1',
        LIMIT = '2',
    };

    enum class SIDE : char {
        BUY = '1',
        SELL = '2',
    };

    enum class STATUS : char {
        NEW = '0',
        PARTIALLY_FILLED = '1',
        FILLED = '2',
        DONE_FOR_DAY = '3',
        CANCELED = '4',
    };
    ///////////////////////////////////////////////////////////////////////////////

protected:
    bool m_isGlobal;
    std::string m_orderID;
    std::string m_brokerID;
    std::string m_traderID;
    std::string m_destination;
    std::unique_ptr<Instrument> m_upInstrument;
    Order::SIDE m_side;
    Order::STATUS m_status;
    double m_orderQuantity;
    double m_leavesQuantity;
    double m_cumulativeQuantity;
    boost::posix_time::ptime m_time;

protected:
    virtual auto cloneImpl() const -> Order* = 0; // clone pattern

public:
    Order(const Order& other); // copy constructor
    auto operator=(const Order& other) & -> Order&; // lvalue-only copy assignment

    Order(Order&&) = default; // move constructor
    auto operator=(Order&&) & -> Order& = default; // lvalue-only move assignment
    virtual ~Order() = default; // virtual destructor

    Order(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time);

    auto clone() const -> std::unique_ptr<Order>; // clone pattern

    virtual Order::TYPE getType() const = 0;

    auto getIsGlobal() const -> bool;
    auto getOrderID() const -> const std::string&;
    auto getBrokerID() const -> const std::string&;
    auto getTraderID() const -> const std::string&;
    auto getDestination() const -> const std::string&;
    auto getInstrument() const -> const Instrument*;
    auto getSide() const -> Order::SIDE;
    auto getStatus() const -> Order::STATUS;
    auto getOrderQuantity() const -> double;
    auto getLeavesQuantity() const -> double;
    auto getCumulativeQuantity() const -> double;
    auto getTime() const -> const boost::posix_time::ptime&;
};
