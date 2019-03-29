#pragma once

#include <list>
#include <string>

#include <quickfix/FieldTypes.h>

/**
 * @brief   Class to dealing with everything about new OrderBook.
 */
class Newbook {
public:
    Newbook();
    Newbook(char _book, std::string _symbol, double _price, int _size, FIX::UtcTimeStamp& _time);
    Newbook(char _book, std::string _symbol, double _price, int _size, std::string _destination, FIX::UtcTimeStamp& _time);
    Newbook(const Newbook& _newbook);
    void store();
    bool empty();
    void get(Newbook& _newbook);
    void copy(const Newbook& _newbook);
    std::string getSymbol();
    double getPrice();
    int getSize();
    FIX::UtcTimeStamp getUtcTime();
    char getBook();
    std::string getDestination();
    virtual ~Newbook();

private:
    std::list<Newbook>::iterator m_listBegin;
    char m_book; //a=local ask, b=local bid, A=global ask, B=global bid
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
