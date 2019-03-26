#include "Price.h"

/**
 * @brief Default constructor for the Newbook class.
 * @param None.
 * @return None.
 */
Price::Price() {}

/**
 * @brief Default destructor for the Newbook class.
 * @param None.
 * @return None.
 */
Price::~Price() {}

/**
 * @brief Setter for price field of current Price object.
 * @param price to be set.
 * @return None.
 */
void Price::setPrice(double _price)
{
    m_price = _price;
}

/**
 * @brief Getter for price field of current Price object.
 * @param None.
 * @return price of the current Price object.
 */
double Price::getPrice()
{
    return m_price;
}

/**
 * @brief Setter for size field of current Price object.
 * @param size to be set.
 * @return None.
 */
void Price::setSize(int _size)
{
    m_size = _size;
}

/**
 * @brief Getter for size field of current Price object.
 * @param None.
 * @return size of the current Price object.
 */
int Price::getSize()
{
    return m_size;
}
