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
    virtual Order* cloneImpl() const = 0; // clone pattern

public:
    Order(const Order& other); // copy constructor
    Order& operator=(const Order& other) &; // copy assignment

    Order(Order&&) = default; // move constructor
    Order& operator=(Order&&) & = default; // move assignment
    virtual ~Order() = default; // virtual destructor

    Order(bool isGlobal, std::string orderID, std::string brokerID, std::string traderID,
        std::string destination, std::unique_ptr<Instrument> instrument, Order::SIDE side, double orderQuantity,
        boost::posix_time::ptime time);

    std::unique_ptr<Order> clone() const; // clone pattern

    virtual Order::TYPE getType() const = 0;

    bool getIsGlobal() const;
    const std::string& getOrderID() const;
    const std::string& getBrokerID() const;
    const std::string& getTraderID() const;
    const std::string& getDestination() const;
    const Instrument* getInstrument() const;
    Order::SIDE getSide() const;
    Order::STATUS getStatus() const;
    double getOrderQuantity() const;
    double getLeavesQuantity() const;
    double getCumulativeQuantity() const;
    const boost::posix_time::ptime& getTime() const;
};
