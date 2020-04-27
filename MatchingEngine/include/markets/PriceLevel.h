#pragma once

#include "Order.h"

#include <list>

namespace markets {

class PriceLevel {
public:
    PriceLevel() = default;
    PriceLevel(const PriceLevel& other) = default;
    virtual ~PriceLevel() = default;

    // getters
    auto getPrice() const -> double;
    auto getSize() const -> int;

    // setters
    void setPrice(double price);
    void setSize(int size);

    auto empty() -> bool;
    void push_back(const Order& order);
    void push_front(const Order& order);

    auto begin() -> std::list<Order>::iterator;
    auto end() -> std::list<Order>::iterator;
    auto erase(std::list<Order>::iterator iter) -> std::list<Order>::iterator;

private:
    double m_price;
    int m_size;
    std::list<Order> m_orders;
};

} // markets