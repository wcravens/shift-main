#pragma once

#include "Order.h"

#include <list>

namespace markets {

class PriceLevel {
public:
    // getters
    double getPrice() const;
    int getSize() const;

    // setters
    void setPrice(double price);
    void setSize(int size);

    bool empty();
    void push_back(const Order& order);
    void push_front(const Order& order);

    std::list<Order>::iterator begin();
    std::list<Order>::iterator end();
    std::list<Order>::iterator erase(std::list<Order>::iterator iter);

private:
    double m_price;
    int m_size;
    std::list<Order> m_orders;
};

} // markets