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
    auto operator=(const PriceLevel&) & -> PriceLevel& = default; // lvalue-only copy assignment
    PriceLevel(PriceLevel&&) = default; // move constructor
    auto operator=(PriceLevel&&) & -> PriceLevel& = default; // lvalue-only move assignment
    virtual ~PriceLevel() = default; // virtual destructor

    PriceLevel(double price);

    auto getPrice() const -> double;
    auto Quantity() -> double&;
    auto DisplayQuantity() -> double&;
    auto Orders() -> std::list<std::unique_ptr<Order>>&;
};
