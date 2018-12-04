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

    OrderBookEntry(double price, int size, const std::string& destination, double time);

    // Getters
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    double getTime() const;

    // Setters
    void setPrice(double price);
    void setSize(int size);
    void setDestination(const std::string& destination);
    void setTime(double time);

private:
    double m_price;
    int m_size;
    std::string m_destination;
    double m_time;
};

} // shift
