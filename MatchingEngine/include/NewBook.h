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
    NewBook(char book, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time);
    NewBook(char book, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time);

    // Getters
    char getBook() const;
    const std::string& getSymbol() const;
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    const FIX::UtcTimeStamp& getUTCTime() const;

private:
    char m_book; // 'a' = local ask, 'b' = local bid, 'A' = global ask, 'B' = global bid
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_time;
};
