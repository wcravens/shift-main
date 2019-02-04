#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

/**
 * @brief   Class that maintains an instance of list of orders, 
 *          and provides options-contract related controls.
 */
class OrderBookEntry {
public:
    enum class ORDER_BOOK_TYPE : char {
        GLB_ASK = 'A',
        GLB_BID = 'B',
        LOC_ASK = 'a',
        LOC_BID = 'b',
        OTHER = 'e'
    };

    OrderBookEntry();
    OrderBookEntry(ORDER_BOOK_TYPE type, std::string symbol, double price, double size, std::string destination, FIX::UtcDateOnly date, FIX::UtcTimeOnly time);

    ORDER_BOOK_TYPE getType() const;
    const std::string& getSymbol() const;
    double getPrice() const;
    double getSize() const;
    const std::string& getDestination() const;
    const FIX::UtcDateOnly getDate() const;
    const FIX::UtcTimeOnly getTime() const;

    static ORDER_BOOK_TYPE s_toOrderBookType(char c);

private:
    ORDER_BOOK_TYPE m_type; // The versioning of the Option Constract of the order book. Update type : a=local ask, b=local bid, A=global ask, B=global bid, smaller ask, bigger bid
    std::string m_symbol; // The symbol of the order.
    double m_price; // The price of the order.
    double m_size; // The size of the order
    std::string m_destination; // The destination of the order.
    FIX::UtcDateOnly m_date;
    FIX::UtcTimeOnly m_time;
};
