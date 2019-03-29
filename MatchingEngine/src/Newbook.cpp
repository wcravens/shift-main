#include "Newbook.h"

#include <mutex>

namespace nameNewbook {
std::list<Newbook> bookupdate;
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
Newbook::Newbook(char _book,
    std::string _symbol,
    double _price,
    int _size,
    FIX::UtcTimeStamp& _time)
    : m_book(_book)
    , m_symbol(std::move(_symbol))
    , m_price(_price)
    , m_size(_size)
    , m_destination("SHIFT")
    , m_time(std::move(_time))
{
}

/**
 * @brief   Constructor for the Newbook class.
 * @param   book, symbol, price, size, time, destination
 */
Newbook::Newbook(char _book,
    std::string _symbol,
    double _price,
    int _size,
    std::string _destination,
    FIX::UtcTimeStamp& _time)
    : m_book(_book)
    , m_symbol(std::move(_symbol))
    , m_price(_price)
    , m_size(_size)
    , m_destination(_destination)
    , m_time(std::move(_time))
{
}

/**
 * @brief   Constructor for the Newbook class.
 * @param   the address of an exist Newbook object
 */
Newbook::Newbook(const Newbook& _newbook)
    : m_book(_newbook.m_book)
    , m_symbol(_newbook.m_symbol)
    , m_price(_newbook.m_price)
    , m_size(_newbook.m_size)
    , m_destination(_newbook.m_destination)
    , m_time(_newbook.m_time)
{
}

/**
 * @brief   Function to push the current Newbook object into the orderbook list.
 */
void Newbook::store()
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    nameNewbook::bookupdate.push_back(*this);
}

/**
 * @brief   Function to check whether current Newbook object is empty.
 * @return  True if empty, else false.
 */
bool Newbook::empty()
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    bool _empty = nameNewbook::bookupdate.empty();
    return _empty;
}

/**
 * @brief   Get the Newbook object from specific address.
 * @param   An exist Newbook object.
 */
void Newbook::get(Newbook& _newbook)
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    m_listBegin = nameNewbook::bookupdate.begin();
    _newbook.copy(*m_listBegin);
    nameNewbook::bookupdate.pop_front();
}

/**
 * @brief   Make a copy of an existing Newbook object.
 * @param   An exist Newbook object.
 */
void Newbook::copy(const Newbook& _newbook)
{
    m_book = _newbook.m_book;
    m_symbol = _newbook.m_symbol;
    m_price = _newbook.m_price;
    m_size = _newbook.m_size;
    m_destination = _newbook.m_destination;
    m_time = _newbook.m_time;
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
FIX::UtcTimeStamp Newbook::getUtcTime() { return m_time; }

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