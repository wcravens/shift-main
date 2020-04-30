#pragma once

#include "CoreClient_EXPORTS.h"

#include <chrono>
#include <ctime>
#include <string>

#include <quickfix/FieldTypes.h>

namespace shift {

/**
 * @brief A class to save order book entries information and package them into an shift::OrderBookEntry object.
 */
class CORECLIENT_EXPORTS OrderBookEntry {
public:
    OrderBookEntry();
    OrderBookEntry(double price, int size, std::string destination, std::chrono::system_clock::time_point time);

    // Getters
    auto getPrice() const -> double;
    auto getSize() const -> int;
    auto getDestination() const -> const std::string&;
    auto getTime() const -> const std::chrono::system_clock::time_point&;

    // Setters
    void setPrice(double price);
    void setSize(int size);
    void setDestination(const std::string& destination);
    void setTime(const std::chrono::system_clock::time_point& time);

private:
    double m_price;
    int m_size;
    std::string m_destination;
    std::chrono::system_clock::time_point m_time;
};

} // shift
