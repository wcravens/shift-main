#pragma once

#include <list>
#include <string>

/**
 * @brief   Class to dealing with everything about new OrderBook.
 */
class Newbook {
public:
    Newbook();
    Newbook(char _book, std::string _symbol, double _price, int _size, double _time);
    Newbook(char _book, std::string _symbol, double _price, int _size, double _time, std::string _destination);
    Newbook(const Newbook& newbook);
    void store();
    bool empty();
    void get(Newbook& newbook);
    void copy(const Newbook& newbook);
    std::string getsymbol();
    double getprice();
    int getsize();
    double gettime();
    char getbook();
    std::string getdestination();
    virtual ~Newbook();

protected:
private:
    std::list<Newbook>::iterator listbegin;
    char book; //a=local ask, b=local bid, A=global ask, B=global bid
    std::string symbol;
    double price;
    int size;
    double time;
    std::string destination;
};
