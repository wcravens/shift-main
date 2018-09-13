#include "BestPrice.h"

/**
 * @brief Default constructor for BestPrice object.
 */
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

/**
 * @brief Constructor with all members preset.
 * @param globalBidPrice double value to set in m_globalBidPrice
 * @param globalBidSize int value to set in m_globalBidSize
 * @param globalAskPrice double value to set in m_globalAskPrice
 * @param globalAskSize int value to set in m_globalAskSize
 * @param localBidPrice double value to set in m_localBidPrice
 * @param localBidSize int value to set in m_localBidSize
 * @param localAskPrice double value to set in m_localAskPrice
 * @param localAskSize int value to set in m_localAskSize
 */
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

/**
 * @brief Getter for the global bid price.
 * @return Global bid price as a double.
 */
double shift::BestPrice::getGlobalBidPrice() const
{
    return m_globalBidPrice;
}

/**
 * @brief Getter for the global bid size.
 * @return Global bid size as an int.
 */
int shift::BestPrice::getGlobalBidSize() const
{
    return m_globalBidSize;
}

/**
 * @brief Getter for the global ask price.
 * @return Global ask price as a double.
 */
double shift::BestPrice::getGlobalAskPrice() const
{
    return m_globalAskPrice;
}

/**
 * @brief Getter for the global ask size.
 * @return Global ask size as an int.
 */
int shift::BestPrice::getGlobalAskSize() const
{
    return m_globalAskSize;
}

/**
 * @brief Getter for the local bid price.
 * @return Local bid price as a double.
 */
double shift::BestPrice::getLocalBidPrice() const
{
    return m_localBidPrice;
}

/**
 * @brief Getter for the local bid size.
 * @return Local bid size as an int.
 */
int shift::BestPrice::getLocalBidSize() const
{
    return m_localBidSize;
}

/**
 * @brief Getter for the local ask price.
 * @return Local ask price as a double.
 */
double shift::BestPrice::getLocalAskPrice() const
{
    return m_localAskPrice;
}

/**
 * @brief Getter for the local ask size.
 * @return Local ask size as an int.
 */
int shift::BestPrice::getLocalAskSize() const
{
    return m_localAskSize;
}
