#include "PriceLevel.h"

PriceLevel::PriceLevel(double price)
    : m_price(price)
    , m_quantity(0.0)
    , m_displayQuantity(0.0)
{
}

double PriceLevel::getPrice() const
{
    return m_price;
}

double& PriceLevel::Quantity()
{
    return m_quantity;
}

double& PriceLevel::DisplayQuantity()
{
    return m_displayQuantity;
}

std::list<std::unique_ptr<Order>>& PriceLevel::Orders()
{
    return m_orders;
}
