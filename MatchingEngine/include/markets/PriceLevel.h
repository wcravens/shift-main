#pragma once

#include "Order.h"

#include <list>

namespace markets {

class PriceLevel {
public:
    PriceLevel() = default;

    // getters
    auto getPrice() const -> double;
    auto getSize() const -> int;
    auto getNumOrders() const -> int;

    // setters
    void setPrice(double price);
    void setSize(int size);

    auto empty() -> bool;
    void push_back(const Order& order);
    void push_front(const Order& order);

    auto begin() -> std::list<Order>::iterator;
    auto begin() const -> std::list<Order>::const_iterator;

    auto end() -> std::list<Order>::iterator;
    auto end() const -> std::list<Order>::const_iterator;

    auto erase(std::list<Order>::iterator iter) -> std::list<Order>::iterator;

    template <class Compare>
    void sort(Compare comp)
    {
        m_orders.sort(comp);
    }

private:
    double m_price;
    int m_size;
    std::list<Order> m_orders;
};

} // markets