#include "Price.h"

/**
 * @brief Default constructor for the Newbook class.
 * @param None.
 * @return None.
 */
Price::Price(void)
{
}

/**
 * @brief Default destructor for the Newbook class.
 * @param None.
 * @return None.
 */
Price::~Price(void)
{
}

/**
 * @brief Setter for price field of current Price object.
 * @param price to be set.
 * @return None.
 */
void Price::setprice(double price1)
{
    price = price1;
}

/**
 * @brief Getter for price field of current Price object.
 * @param None.
 * @return price of the current Price object.
 */
double Price::getprice()
{
    return price;
}

/**
 * @brief Setter for size field of current Price object.
 * @param size to be set.
 * @return None.
 */
void Price::setsize(int size1)
{
    size = size1;
}

/**
 * @brief Getter for size field of current Price object.
 * @param None.
 * @return size of the current Price object.
 */
int Price::getsize()
{
    return size;
}
