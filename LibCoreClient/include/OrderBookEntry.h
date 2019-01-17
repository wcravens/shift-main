#pragma once

#include <quickfix/FieldTypes.h>

#include "CoreClient_EXPORTS.h"
#include <string>

namespace shift {

/**
 * @brief A class to save order book entries information and package them into an shift::OrderBookEntry object.
 */
class CORECLIENT_EXPORTS OrderBookEntry {

public:
    OrderBookEntry();

    OrderBookEntry(double price, int size, const std::string& destination, FIX::UtcTimeStamp utctime);

    // Getters
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    const FIX::UtcTimeStamp getUtcTime() const;

    // Setters
    void setPrice(double price);
    void setSize(int size);
    void setDestination(const std::string& destination);
    void setUtcTime(FIX::UtcTimeStamp utctime);

private:
    double m_price;
    int m_size;
    std::string m_destination;
    FIX::UtcTimeStamp m_utctime;
};

} // shift
