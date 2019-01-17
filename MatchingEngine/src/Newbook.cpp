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
Newbook::Newbook(char _book, std::string _symbol, double _price, int _size, FIX::UtcTimeStamp& _utctime)
{
    book = _book;
    symbol = _symbol;
    price = _price;
    size = _size;
    destination = "Server";
    utctime = _utctime;
}

/**
 * @brief   Constructor for the Newbook class.
 * @param   book, symbol, price, size, time, destination
 */
Newbook::Newbook(char _book, std::string _symbol, double _price, int _size, std::string _destination, FIX::UtcTimeStamp& _utctime)
{
    book = _book;
    symbol = _symbol;
    price = _price;
    size = _size;
    destination = _destination;
    utctime = _utctime;
}

/**
 * @brief   Constructor for the Newbook class.
 * @param   the address of an exist Newbook object
 */
Newbook::Newbook(const Newbook& newbook)
{
    book = newbook.book;
    symbol = newbook.symbol;
    price = newbook.price;
    size = newbook.size;
    destination = newbook.destination;
    utctime = newbook.utctime;
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
void Newbook::get(Newbook& newbook)
{
    std::lock_guard<std::mutex> lock(nameNewbook::newbookMu);
    listbegin = nameNewbook::bookupdate.begin();
    newbook.copy(*listbegin);
    nameNewbook::bookupdate.pop_front();
}

/**
 * @brief   Make a copy of an existing Newbook object.
 * @param   An exist Newbook object.
 */
void Newbook::copy(const Newbook& newbook)
{
    book = newbook.book;
    symbol = newbook.symbol;
    price = newbook.price;
    size = newbook.size;
    destination = newbook.destination;
    utctime = newbook.utctime;
}

/**
 * @brief   Getter function for the field of symbol.
 * @return  The symbol of the current order book object.
 */
std::string Newbook::getsymbol() { return symbol; }

/**
 * @brief   Getter function for the field of price.
 * @return  The price of the current order book object.
 */
double Newbook::getprice() { return price; }

/**
 * @brief   Getter function for the field of size.
 * @return  The size of the current order book object.
 */
int Newbook::getsize() { return size; }

/**
 * @brief   Getter function for the field of time.
 * @return  The time of the current order book object.
 */
FIX::UtcTimeStamp Newbook::getutctime() { return utctime; }

/**
 * @brief   Getter function for the field of book.
 * @return  The book of the current order book object.
 */
char Newbook::getbook() { return book; }

/**
 * @brief   Getter function for the field of destination.
 * @return  The destination of the current order book object.
 */
std::string Newbook::getdestination() { return destination; }

/**
 * @brief   Default destructor for the Newbook class.
 */
Newbook::~Newbook()
{
    //dtor
}