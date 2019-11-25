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

    OrderBookEntry(double price, int size, const std::string& destination, std::chrono::system_clock::time_point time);

    // Getters
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    const std::chrono::system_clock::time_point getTime() const;

    // Setters
    void setPrice(double price);
    void setSize(int size);
    void setDestination(const std::string& destination);
    void setTime(std::chrono::system_clock::time_point time);

private:
    double m_price;
    int m_size;
    std::string m_destination;
    std::chrono::system_clock::time_point m_time;
};

} // shift
