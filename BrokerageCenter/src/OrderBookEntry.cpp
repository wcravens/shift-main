#include "OrderBookEntry.h"

#include <list>
#include <mutex>

namespace new_book {
std::list<OrderBookEntry> bookUpdate;
std::mutex mtxNewBook;
}

/**
 * @brief   Default constructs an OrderBookEntry instance.
 */
OrderBookEntry::OrderBookEntry() = default;

/**
 * @brief   Constructs with corresponding parameters.
 * @param   type: The versioning of the Option Constract.
 * @param	symbol: The symbol of the order.
 * @param   price: The price of order.
 * @param	size: The contract size.
 * @param   time: The time passed by.
 * @param   destination: The security ID used as destination.
 */
OrderBookEntry::OrderBookEntry(ORDER_BOOK_TYPE type, std::string symbol, double price, double size, double time, std::string destination)
    : m_type(type)
    , m_symbol(std::move(symbol))
    , m_price{ price }
    , m_size{ size }
    , m_time{ time }
    , m_destination(std::move(destination))
{
}

/**
 * @brief   Getter of the orderbook type.
 * @return  The orderbook type.
 */
auto OrderBookEntry::getType() const -> ORDER_BOOK_TYPE
{
    return m_type;
}

/**
 * @brief   Getter of the symbol of order.
 * @return  The symbol of order.
 */
const std::string& OrderBookEntry::getSymbol() const
{
    return m_symbol;
}

/**
 * @brief   Getter of the strike price of the constract.
 * @return  The order price.
 */
double OrderBookEntry::getPrice() const
{
    return m_price;
}

/**
 * @brief   Getter of the contract size.
 * @return  The size.
 */
double OrderBookEntry::getSize() const
{
    return m_size;
}

/**
 * @brief   Getter of the time of order.
 * @return  The time.
 */
double OrderBookEntry::getTime() const
{
    return m_time;
}

/**
 * @brief  Getter of security ID.
 * @return Security ID (as the destination).
 */
const std::string& OrderBookEntry::getDestination() const
{
    return m_destination;
}

/**
 * @brief   Convert character to ORDER_BOOK_TYPE
 */
/*static*/ auto OrderBookEntry::s_toOrderBookType(char c) -> ORDER_BOOK_TYPE
{
    using OBT = ORDER_BOOK_TYPE;

    switch (c) {
    case 'A':
        return OBT::GLB_ASK;
    case 'B':
        return OBT::GLB_BID;
    case 'a':
        return OBT::LOC_ASK;
    case 'b':
        return OBT::LOC_BID;
    default:
        return OBT::OTHER;
    }
}
