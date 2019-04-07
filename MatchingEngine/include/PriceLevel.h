#pragma once

#include "Quote.h"

#include <list>

class PriceLevel {
public:
    // Getters
    double getPrice() const;
    int getSize() const;

    // Setters
    void setPrice(double price);
    void setSize(int size);

    bool empty();
    void push_back(const Quote& quote);
    void push_front(const Quote& quote);

    std::list<Quote>::iterator begin();
    std::list<Quote>::iterator end();
    std::list<Quote>::iterator erase(std::list<Quote>::iterator iter);

private:
    double m_price;
    int m_size;
    std::list<Quote> m_quotes;
};
