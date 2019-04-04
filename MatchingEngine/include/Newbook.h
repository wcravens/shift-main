#pragma once

#include <list>
#include <string>

#include <quickfix/FieldTypes.h>

/**
 * @brief   Class to dealing with everything about new OrderBook.
 */
class NewBook {
public:
    NewBook() = default;
    virtual ~NewBook() = default;

    NewBook(char book, std::string symbol, double price, int size, FIX::UtcTimeStamp& time);
    NewBook(char book, std::string symbol, double price, int size, std::string destination, FIX::UtcTimeStamp& time);
    NewBook(const NewBook& other);

    void store();
    bool empty();
    void get(NewBook& other);
    void copy(const NewBook& other);
    std::string getSymbol();
    double getPrice();
    int getSize();
    FIX::UtcTimeStamp getUTCTime();
    char getBook();
    std::string getDestination();

private:
    std::list<NewBook>::iterator m_listBegin;
    char m_book; // 'a' = local ask, 'b' = local bid, 'A' = global ask, 'B' = global bid
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
