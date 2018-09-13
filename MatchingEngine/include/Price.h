#pragma once

#include "Quote.h"

#include <list>

/**
 * @brief A class for all Price object.
 */
class Price {
private:
    double price;
    int size;
    typedef std::list<Quote> ListQuote;

    //ListQuote::iterator thisquote;

public:
    ListQuote quotes;
    //void add(Quote newquote);
    //void del(Quote){}

    Price(void);
    ~Price(void);

    void setprice(double price1);
    double getprice();

    void setsize(int size1);
    int getsize();
};
