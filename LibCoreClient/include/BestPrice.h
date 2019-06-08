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
    double m_globalBidPrice;
    int m_globalBidSize;

    double m_globalAskPrice;
    int m_globalAskSize;

    double m_localBidPrice;
    int m_localBidSize;

    double m_localAskPrice;
    int m_localAskSize;
};

} // shift
