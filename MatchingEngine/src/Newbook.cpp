#include "Newbook.h"

#include <mutex>

namespace nameNewbook {
std::list<Newbook> bookUpdate;
std::mutex newbookMu;
}

/**
 * @brief   Default constructor for the Newbook class.
 */
Newbook::Newbook() {}

/**
 * @brief   Constructor for the Newbook class.
 * @param   book, symbol, price, size, time
 */
Newbook::Newbook(char book,
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
 * @brief   Constructor for the Newbook class.
 * @param   book, symbol, price, size, time, destination
 */
Newbook::Newbook(char book,
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
 * @brief   Constructor for the Newbook class.
 * @param   the address of an exist Newbook object
 */
Newbook::Newbook(const Newbook& other)
    : m_book(other.m_book)
    , m_symbol(other.m_symbol)
    , m_price(other.m_price)
    , m_size(other.m_size)
    , m_destination(other.m_destination)
    , m_time(other.m_time)
{
}

/**
 * @brief   Function to push the current Newbook object into the orderbook list.
 */
void Newbook::store()
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    nameNewbook::bookUpdate.push_back(*this);
}

/**
 * @brief   Function to check whether current Newbook object is empty.
 * @return  True if empty, else false.
 */
bool Newbook::empty()
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    bool empty = nameNewbook::bookUpdate.empty();
    return empty;
}

/**
 * @brief   Get the Newbook object from specific address.
 * @param   An exist Newbook object.
 */
void Newbook::get(Newbook& other)
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    m_listBegin = nameNewbook::bookUpdate.begin();
    other.copy(*m_listBegin);
    nameNewbook::bookUpdate.pop_front();
}

/**
 * @brief   Make a copy of an existing Newbook object.
 * @param   An exist Newbook object.
 */
void Newbook::copy(const Newbook& other)
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
std::string Newbook::getSymbol() { return m_symbol; }

/**
 * @brief   Getter function for the field of price.
 * @return  The price of the current order book object.
 */
double Newbook::getPrice() { return m_price; }

/**
 * @brief   Getter function for the field of size.
 * @return  The size of the current order book object.
 */
int Newbook::getSize() { return m_size; }

/**
 * @brief   Getter function for the field of time.
 * @return  The time of the current order book object.
 */
FIX::UtcTimeStamp Newbook::getUTCTime() { return m_time; }

/**
 * @brief   Getter function for the field of book.
 * @return  The book of the current order book object.
 */
char Newbook::getBook() { return m_book; }

/**
 * @brief   Getter function for the field of destination.
 * @return  The destination of the current order book object.
 */
std::string Newbook::getDestination() { return m_destination; }

/**
 * @brief   Default destructor for the Newbook class.
 */
Newbook::~Newbook()
{
    //dtor
}