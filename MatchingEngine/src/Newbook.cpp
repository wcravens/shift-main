#include "NewBook.h"

#include <mutex>

namespace nameNewBook {
std::list<NewBook> bookUpdate;
std::mutex newBookMu;
}

/**
 * @brief   Constructor for the NewBook class.
 * @param   book, symbol, price, size, time
 */
NewBook::NewBook(char book,
    std::string symbol,
    double price,
    int size,
    FIX::UtcTimeStamp& time)
    : m_book(book)
    , m_symbol(std::move(symbol))
    , m_price(price)
    , m_size(size)
    , m_destination("SHIFT")
    , m_time(std::move(time))
{
}

/**
 * @brief   Constructor for the NewBook class.
 * @param   book, symbol, price, size, time, destination
 */
NewBook::NewBook(char book,
    std::string symbol,
    double price,
    int size,
    std::string destination,
    FIX::UtcTimeStamp& time)
    : m_book(book)
    , m_symbol(std::move(symbol))
    , m_price(price)
    , m_size(size)
    , m_destination(destination)
    , m_time(std::move(time))
{
}

/**
 * @brief   Constructor for the NewBook class.
 * @param   the address of an exist NewBook object
 */
NewBook::NewBook(const NewBook& other)
    : m_book(other.m_book)
    , m_symbol(other.m_symbol)
    , m_price(other.m_price)
    , m_size(other.m_size)
    , m_destination(other.m_destination)
    , m_time(other.m_time)
{
}

/**
 * @brief   Function to push the current NewBook object into the orderbook list.
 */
void NewBook::store()
{
    std::lock_guard<std::mutex> lock(nameNewBook::newBookMu);
    nameNewBook::bookUpdate.push_back(*this);
}

/**
 * @brief   Function to check whether current NewBook object is empty.
 * @return  True if empty, else false.
 */
bool NewBook::empty()
{
    std::lock_guard<std::mutex> lock(nameNewBook::newBookMu);
    bool empty = nameNewBook::bookUpdate.empty();
    return empty;
}

/**
 * @brief   Get the NewBook object from specific address.
 * @param   An exist NewBook object.
 */
void NewBook::get(NewBook& other)
{
    std::lock_guard<std::mutex> lock(nameNewBook::newBookMu);
    m_listBegin = nameNewBook::bookUpdate.begin();
    other.copy(*m_listBegin);
    nameNewBook::bookUpdate.pop_front();
}

/**
 * @brief   Make a copy of an existing NewBook object.
 * @param   An exist NewBook object.
 */
void NewBook::copy(const NewBook& other)
{
    m_book = other.m_book;
    m_symbol = other.m_symbol;
    m_price = other.m_price;
    m_size = other.m_size;
    m_destination = other.m_destination;
    m_time = other.m_time;
}

/**
 * @brief   Getter function for the field of symbol.
 * @return  The symbol of the current order book object.
 */
std::string NewBook::getSymbol() { return m_symbol; }

/**
 * @brief   Getter function for the field of price.
 * @return  The price of the current order book object.
 */
double NewBook::getPrice() { return m_price; }

/**
 * @brief   Getter function for the field of size.
 * @return  The size of the current order book object.
 */
int NewBook::getSize() { return m_size; }

/**
 * @brief   Getter function for the field of time.
 * @return  The time of the current order book object.
 */
FIX::UtcTimeStamp NewBook::getUTCTime() { return m_time; }

/**
 * @brief   Getter function for the field of book.
 * @return  The book of the current order book object.
 */
char NewBook::getBook() { return m_book; }

/**
 * @brief   Getter function for the field of destination.
 * @return  The destination of the current order book object.
 */
std::string NewBook::getDestination() { return m_destination; }
