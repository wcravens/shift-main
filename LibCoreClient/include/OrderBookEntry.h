#pragma once

#include "CoreClient_EXPORTS.h"
#include <string>

namespace shift {

/**
 * @brief A class to save order book entries information and package them into an shift::OrderBookEntry object.
 */
class CORECLIENT_EXPORTS OrderBookEntry {

public:
    OrderBookEntry();

    OrderBookEntry(double price, int size, double time, const std::string& destination);

    // Getters
    double getPrice() const;
    int getSize() const;
    double getTime() const;
    const std::string& getDestination() const;

    // Setters
    void setPrice(double price);
    void setSize(int size);
    void setTime(double time);
    void setDestination(const std::string& destination);

private:
    double m_price;
    int m_size;
    double m_time;
    std::string m_destination;
};

} // shift
