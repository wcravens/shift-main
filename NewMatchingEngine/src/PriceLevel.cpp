#include "PriceLevel.h"

PriceLevel::PriceLevel(double price)
    : m_price { price }
    , m_quantity { 0.0 }
    , m_displayQuantity { 0.0 }
{
}

auto PriceLevel::getPrice() const -> double
{
    return m_price;
}

auto PriceLevel::Quantity() -> double&
{
    return m_quantity;
}

auto PriceLevel::DisplayQuantity() -> double&
{
    return m_displayQuantity;
}

auto PriceLevel::Orders() -> std::list<std::unique_ptr<Order>>&
{
    return m_orders;
}
