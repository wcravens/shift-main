#pragma once

#include "CoreClient_EXPORTS.h"

namespace shift {

/**
 * @brief A class to save all price/size info for both global and local, ask and bid.
 */
class CORECLIENT_EXPORTS BestPrice {

public:
    BestPrice();

    BestPrice(double globalBidPrice, int globalBidSize, double globalAskPrice, int globalAskSize,
        double localBidPrice, int localBidSize, double localAskPrice, int localAskSize);

    ~BestPrice() = default;

    double getBidPrice() const;
    int getBidSize() const;

    double getAskPrice() const;
    int getAskSize() const;

    double getGlobalBidPrice() const;
    int getGlobalBidSize() const;

    double getGlobalAskPrice() const;
    int getGlobalAskSize() const;

    double getLocalBidPrice() const;
    int getLocalBidSize() const;

    double getLocalAskPrice() const;
    int getLocalAskSize() const;

private:
    double m_globalBidPrice; //!< Global bid price for current BestPrice object
    int m_globalBidSize; //!< Size of orders with the best global bid price.

    double m_globalAskPrice; //!< Global ask price for current BestPrice object
    int m_globalAskSize; //!< Size of orders with the best global ask price.

    double m_localBidPrice; //!< Local bid price for current BestPrice object
    int m_localBidSize; //!< Size of orders with the best local bid price.

    double m_localAskPrice; //!< Local ask price for current BestPrice object
    int m_localAskSize; //!< Size of orders with the best local ask price.
};

} // shift
