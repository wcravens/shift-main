#pragma once

#include "Quote.h"

#include <list>

/**
 * @brief A class for all Price object.
 */
class Price {
private:
    typedef std::list<Quote> ListQuote;
    typedef ListQuote::iterator Iterator;

private:
    double m_price;
    int m_size;
    ListQuote m_quotes;

public:
    Price();
    ~Price();

    void setPrice(double _price);
    double getPrice();

    void setSize(int _size);
    int getSize();

    Iterator begin() { return m_quotes.begin(); }
    Iterator end() { return m_quotes.end(); }
    Iterator erase(Iterator _iter) { return m_quotes.erase(_iter); }
    bool empty() { return m_quotes.empty(); }
    void push_back(const Quote& _quote) { m_quotes.push_back(_quote); }
    void push_front(const Quote& _quote) { m_quotes.push_front(_quote); }
};
