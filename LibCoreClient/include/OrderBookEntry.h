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

    OrderBookEntry(char type, std::string symbol, double price, int size,
        std::string destination, double time);

    // Getters
    char getType() const;
    const std::string& getSymbol() const;
    double getPrice() const;
    int getSize() const;
    const std::string& getDestination() const;
    double getTime() const;

    // Setters
    void setType(char type);
    void setSymbol(std::string symbol);
    void setPrice(double price);
    void setSize(int size);
    void setDestination(std::string destination);
    void setTime(double time);

private:
    char m_type; //!< Order type
    std::string m_symbol;
    double m_price;
    int m_size;
    std::string m_destination;
    double m_time; //!< Order executing time
};

} // shift
