#include "NewBook.h"

/**
 * @brief Constructor for the NewBook class.
 * @param book, symbol, price, size, time
 */
NewBook::NewBook(char book, const std::string& symbol, double price, int size, const FIX::UtcTimeStamp& time)
    : m_book(book)
    , m_symbol(symbol)
    , m_price(price)
    , m_size(size)
    , m_destination("SHIFT")
    , m_time(time)
{
}

/**
 * @brief Constructor for the NewBook class.
 * @param book, symbol, price, size, time, destination
 */
NewBook::NewBook(char book, const std::string& symbol, double price, int size, const std::string& destination, const FIX::UtcTimeStamp& time)
    : m_book(book)
    , m_symbol(symbol)
    , m_price(price)
    , m_size(size)
    , m_destination(destination)
    , m_time(time)
{
}

char NewBook::getBook() const
{
    return m_book;
}

const std::string& NewBook::getSymbol() const
{
    return m_symbol;
}

double NewBook::getPrice() const
{
    return m_price;
}

int NewBook::getSize() const
{
    return m_size;
}

const std::string& NewBook::getDestination() const
{
    return m_destination;
}

const FIX::UtcTimeStamp& NewBook::getUTCTime() const
{
    return m_time;
}
