#pragma once

#include "Order.h"

#include <list>

class PriceLevel {

protected:
    double m_price;
    double m_quantity;
    double m_displayQuantity;
    std::list<std::unique_ptr<Order>> m_orders;

public:
    PriceLevel(const PriceLevel&) = default; // copy constructor
    PriceLevel& operator=(const PriceLevel&) & = default; // copy assignment
    PriceLevel(PriceLevel&&) = default; // move constructor
    PriceLevel& operator=(PriceLevel&&) & = default; // move assignment
    virtual ~PriceLevel() = default; // virtual destructor

    PriceLevel(double price);

    double getPrice() const;
    double& Quantity();
    double& DisplayQuantity();
    std::list<std::unique_ptr<Order>>& Orders();
};
