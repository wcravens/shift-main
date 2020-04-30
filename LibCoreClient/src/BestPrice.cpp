#include "BestPrice.h"

namespace shift {

BestPrice::BestPrice()
    : BestPrice { 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0 }

{
}

BestPrice::BestPrice(const std::pair<double, int>& globalBidValues, const std::pair<double, int>& globalAskValues,
    const std::pair<double, int>& localBidValues, const std::pair<double, int>& localAskValues)
    : BestPrice { globalBidValues.first, globalBidValues.second, globalAskValues.first, globalAskValues.second,
        localBidValues.first, localBidValues.second, localAskValues.first, localAskValues.second }

{
}

BestPrice::BestPrice(double globalBidPrice, int globalBidSize, double globalAskPrice, int globalAskSize,
    double localBidPrice, int localBidSize, double localAskPrice, int localAskSize)
    : m_globalBidPrice { globalBidPrice }
    , m_globalBidSize { globalBidSize }
    , m_globalAskPrice { globalAskPrice }
    , m_globalAskSize { globalAskSize }
    , m_localBidPrice { localBidPrice }
    , m_localBidSize { localBidSize }
    , m_localAskPrice { localAskPrice }
    , m_localAskSize { localAskSize }
{
}

/**
 * @brief Getter to get the current best bid price.
 * @return Current best bid price as double.
 */
auto BestPrice::getBidPrice() const -> double
{
    // pick the highest between global bid price and local bid price
    if (m_globalBidPrice > m_localBidPrice) {
        return m_globalBidPrice;
    }

    if (m_globalBidPrice < m_localBidPrice) {
        return m_localBidPrice;
    }

    return m_globalBidPrice;
}

/**
 * @brief Getter to get size for orders with the current best bid.
 * @return Size for orders with the current best bid price as int.
 */
auto BestPrice::getBidSize() const -> int
{
    if (m_globalBidPrice > m_localBidPrice) {
        return m_globalBidSize;
    }

    if (m_globalBidPrice < m_localBidPrice) {
        return m_localBidSize;
    }

    return m_globalBidSize + m_localBidSize;
}

/**
 * @brief Getter to get the current best ask price.
 * @return Current best ask price as double.
 */
auto BestPrice::getAskPrice() const -> double
{
    // pick the lowest between global ask price and local ask price
    if ((m_globalAskPrice > 0.0 && m_globalAskPrice < m_localAskPrice) || (m_localAskPrice == 0.0)) {
        return m_globalAskPrice;
    }

    if ((m_localAskPrice > 0.0 && m_globalAskPrice > m_localAskPrice) || (m_globalAskPrice == 0.0)) {
        return m_localAskPrice;
    }

    return m_globalAskPrice;
}

/**
 * @brief Getter to get size for orders with the current best ask.
 * @return Size for orders with the current best ask price as int.
 */
auto BestPrice::getAskSize() const -> int
{
    if ((m_globalAskPrice > 0.0 && m_globalAskPrice < m_localAskPrice) || (m_localAskPrice == 0.0)) {
        return m_globalAskSize;
    }

    if ((m_localAskPrice > 0.0 && m_globalAskPrice > m_localAskPrice) || (m_globalAskPrice == 0.0)) {
        return m_localAskSize;
    }

    return m_globalAskSize + m_localAskSize;
}

auto BestPrice::getGlobalBidPrice() const -> double
{
    return m_globalBidPrice;
}

auto BestPrice::getGlobalBidSize() const -> int
{
    return m_globalBidSize;
}

auto BestPrice::getGlobalAskPrice() const -> double
{
    return m_globalAskPrice;
}

auto BestPrice::getGlobalAskSize() const -> int
{
    return m_globalAskSize;
}

auto BestPrice::getLocalBidPrice() const -> double
{
    return m_localBidPrice;
}

auto BestPrice::getLocalBidSize() const -> int
{
    return m_localBidSize;
}

auto BestPrice::getLocalAskPrice() const -> double
{
    return m_localAskPrice;
}

auto BestPrice::getLocalAskSize() const -> int
{
    return m_localAskSize;
}

} // shift
