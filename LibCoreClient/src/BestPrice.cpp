#include "BestPrice.h"

shift::BestPrice::BestPrice()
    : m_globalBidPrice(0.0)
    , m_globalBidSize(0)
    , m_globalAskPrice(0.0)
    , m_globalAskSize(0)
    , m_localBidPrice(0.0)
    , m_localBidSize(0)
    , m_localAskPrice(0.0)
    , m_localAskSize(0)
{
}

shift::BestPrice::BestPrice(double globalBidPrice, int globalBidSize, double globalAskPrice, int globalAskSize,
    double localBidPrice, int localBidSize, double localAskPrice, int localAskSize)
    : m_globalBidPrice(globalBidPrice)
    , m_globalBidSize(globalBidSize)
    , m_globalAskPrice(globalAskPrice)
    , m_globalAskSize(globalAskSize)
    , m_localBidPrice(localBidPrice)
    , m_localBidSize(localBidSize)
    , m_localAskPrice(localAskPrice)
    , m_localAskSize(localAskSize)
{
}

/**
 * @brief Getter to get the current best bid price.
 * @return Current best bid price as double.
 */
double shift::BestPrice::getBidPrice() const
{
    // Pick the higher one between global bid price and local bid price.
    if (m_globalBidPrice > m_localBidPrice)
        return m_globalBidPrice;

    if (m_globalBidPrice < m_localBidPrice)
        return m_localBidPrice;

    return m_globalBidPrice;
}

/**
 * @brief Getter to get size for orders with the current best bid.
 * @return Size for orders with the current best bid price as int.
 */
int shift::BestPrice::getBidSize() const
{
    if (m_globalBidPrice > m_localBidPrice)
        return m_globalBidSize;

    if (m_globalBidPrice < m_localBidPrice)
        return m_localBidSize;

    return m_globalBidSize + m_localBidSize;
}

/**
 * @brief Getter to get the current best ask price.
 * @return Current best ask price as double.
 */
double shift::BestPrice::getAskPrice() const
{
    // Pick the lower one between global ask price and local ask price.
    if ((m_globalAskPrice && m_globalAskPrice < m_localAskPrice) || !m_localAskPrice)
        return m_globalAskPrice;

    if ((m_localAskPrice && m_globalAskPrice > m_localAskPrice) || !m_globalAskPrice)
        return m_localAskPrice;

    return m_globalAskPrice;
}

/**
 * @brief Getter to get size for orders with the current best ask.
 * @return Size for orders with the current best ask price as int.
 */
int shift::BestPrice::getAskSize() const
{
    if ((m_globalAskPrice && m_globalAskPrice < m_localAskPrice) || !m_localAskPrice)
        return m_globalAskSize;

    if ((m_localAskPrice && m_globalAskPrice > m_localAskPrice) || !m_globalAskPrice)
        return m_localAskSize;

    return m_globalAskSize + m_localAskSize;
}

double shift::BestPrice::getGlobalBidPrice() const
{
    return m_globalBidPrice;
}

int shift::BestPrice::getGlobalBidSize() const
{
    return m_globalBidSize;
}

double shift::BestPrice::getGlobalAskPrice() const
{
    return m_globalAskPrice;
}

int shift::BestPrice::getGlobalAskSize() const
{
    return m_globalAskSize;
}

double shift::BestPrice::getLocalBidPrice() const
{
    return m_localBidPrice;
}

int shift::BestPrice::getLocalBidSize() const
{
    return m_localBidSize;
}

double shift::BestPrice::getLocalAskPrice() const
{
    return m_localAskPrice;
}

int shift::BestPrice::getLocalAskSize() const
{
    return m_localAskSize;
}