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

    auto getBidPrice() const -> double;
    auto getBidSize() const -> int;

    auto getAskPrice() const -> double;
    auto getAskSize() const -> int;

    auto getGlobalBidPrice() const -> double;
    auto getGlobalBidSize() const -> int;

    auto getGlobalAskPrice() const -> double;
    auto getGlobalAskSize() const -> int;

    auto getLocalBidPrice() const -> double;
    auto getLocalBidSize() const -> int;

    auto getLocalAskPrice() const -> double;
    auto getLocalAskSize() const -> int;

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
