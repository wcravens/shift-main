#include "Stock.h"

#include "TimeSetting.h"

#include <shift/miscutils/terminal/Common.h>

Stock::Stock() {}

Stock::Stock(std::string name)
    : m_name(std::move(name))
{
}

Stock::Stock(const Stock& stock)
    : m_name(stock.m_name)
{
}

Stock::~Stock() {}

void Stock::bufNewGlobal(Quote& quote)
{
    try {
        std::lock_guard<std::mutex> lock(m_newGlobal_mu);
        m_newGlobal.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void Stock::bufNewLocal(Quote& quote)
{
    try {
        std::lock_guard<std::mutex> lock(m_newGlobal_mu);
        m_newLocal.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

bool Stock::getNewQuote(Quote& quote)
{
    bool good = false;
    long milli_now = globalTimeSetting.pastMilli(true);

    std::lock_guard<std::mutex> g_lock(m_newGlobal_mu);
    if (!m_newGlobal.empty()) {
        quote = m_newGlobal.front();
        if (quote.getMilli() < milli_now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> l_lock(m_newLocal_mu);
    if (!m_newLocal.empty()) {
        Quote* newlocal = &m_newLocal.front();
        if (good) {
            if (quote.getMilli() >= newlocal->getMilli()) {
                quote = *newlocal;
                m_newLocal.pop();
            } else
                m_newGlobal.pop();
        } else if (newlocal->getMilli() < milli_now) {
            good = true;
            quote = *newlocal;
            m_newLocal.pop();
        }
    } else if (good) {
        m_newGlobal.pop();
    }

    return good;
}

void Stock::setStockname(std::string name)
{
    m_name = std::move(name);
}

std::string Stock::getStockname()
{
    return m_name;
}

///decision1 can be '4' means cancel or '2' means trade, record trade or cancel with object actions, size1=m_thisQuote->getSize()-quote2->getSize()
void Stock::execute(int size1, Quote& newquote, char decision1, std::string destination1)
{
    int executesize = std::min(m_thisQuote->getSize(), newquote.getSize()); //////#include <algorithm>
    m_thisPrice->setSize(m_thisPrice->getSize() - executesize);
    //int newquotesize;

    if (size1 >= 0) {
        m_thisQuote->setSize(size1);
        //newquotesize=0;
        newquote.setSize(0);
        //cout<<newquote.getSize();

    } else {
        m_thisQuote->setSize(0);
        //newquotesize=-size1;
        newquote.setSize(-size1);
        //cout<<newquote.getSize();
    }

    auto utc_now = globalTimeSetting.simulationTimestamp();
    Action act(
        newquote.getStockname(),
        m_thisQuote->getPrice(),
        executesize,
        m_thisQuote->getTraderID(),
        newquote.getTraderID(),
        m_thisQuote->getOrderType(),
        newquote.getOrderType(),
        m_thisQuote->getOrderID(),
        newquote.getOrderID(),
        decision1,
        destination1,
        utc_now,
        m_thisQuote->getTime(),
        newquote.getTime());

    actions.push_back(act);
    //level();
    // cout << "actions:  " << actions.begin()->time1 << actions.begin()->decision << endl;
    //return newquotesize;
}

void Stock::executeGlobal(int size1, Quote& newquote, char decision1, std::string destination1)
{
    int executesize = std::min(m_thisGlobal->getSize(), newquote.getSize());
    if (size1 >= 0) {
        m_thisGlobal->setSize(size1);
        newquote.setSize(0);
    } else {
        m_thisGlobal->setSize(0);
        newquote.setSize(-size1);
    }

    auto utc_now = globalTimeSetting.simulationTimestamp();
    Action act(
        newquote.getStockname(),
        m_thisGlobal->getPrice(),
        executesize,
        m_thisGlobal->getTraderID(),
        newquote.getTraderID(),
        m_thisGlobal->getOrderType(),
        newquote.getOrderType(),
        m_thisGlobal->getOrderID(),
        newquote.getOrderID(),
        decision1,
        destination1,
        utc_now,
        m_thisGlobal->getTime(),
        newquote.getTime());

    actions.push_back(act);
    //level();
    //cout<<"Global Actions happening!!!!"<<endl;
}

///////////////////////////////////////////////do limit buy
void Stock::doLimitBuy(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_ask.begin();
    while (newquote.getSize() != 0) //search list<Price> ask for best price, smaller than newquote
    {
        if (m_thisPrice == m_ask.end())
            break;
        if (m_thisPrice->getPrice() <= newquote.getPrice()) {
            m_thisQuote = m_thisPrice->begin();
            while (newquote.getSize() != 0) {
                if (m_thisQuote == m_thisPrice->end())
                    break;
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
                //cout<<'\n'<<size1;
                //newquote.setSize(execute(size1,newquote,'T',destination1));
                execute(size1, newquote, '2', destination1);
                //cout<<newquote.getSize();
                if (size1 <= 0) {
                    m_thisQuote = m_thisPrice->erase(m_thisQuote);
                }
            }
        }

        bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
            globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

        if (m_thisPrice->empty()) {
            m_thisPrice = m_ask.erase(m_thisPrice);
        } else
            ++m_thisPrice;
    }
    //if(newquote.getSize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the bid list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::doLimitSell(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_bid.begin();
    while (newquote.getSize() != 0) //search list<Price> bid for best price, smaller than newquote
    {
        if (m_thisPrice == m_bid.end())
            break;
        if (m_thisPrice->getPrice() >= newquote.getPrice()) {
            m_thisQuote = m_thisPrice->begin();
            while (newquote.getSize() != 0) {
                if (m_thisQuote == m_thisPrice->end())
                    break;
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
                //cout<<'\n'<<size1;
                //newquote.setSize(execute(size1,newquote,'T',destination1));
                execute(size1, newquote, '2', destination1);
                //cout<<newquote.getSize();
                if (size1 <= 0) {
                    m_thisQuote = m_thisPrice->erase(m_thisQuote);
                }
            }
        }

        bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
            globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

        if (m_thisPrice->empty()) {
            m_thisPrice = m_bid.erase(m_thisPrice);
        } else
            ++m_thisPrice;
    }
    //if(newquote.getSize()!=0){ask_insert(newquote);}
    //level();
}

///////////////////////////////////////////do market buy
void Stock::doMarketBuy(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_ask.begin();
    while (newquote.getSize() != 0) //search list<Price> ask for best price
    {
        if (m_thisPrice == m_ask.end())
            break;
        m_thisQuote = m_thisPrice->begin();
        while (newquote.getSize() != 0) {
            if (m_thisQuote == m_thisPrice->end())
                break;
            if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setSize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }
        }

        bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
            globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

        if (m_thisPrice->empty()) {
            m_thisPrice = m_ask.erase(m_thisPrice);
        } else
            ++m_thisPrice;
    }
    //if(newquote.getSize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::doMarketSell(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_bid.begin();
    while (newquote.getSize() != 0) //search list<Price> bid for best price
    {
        if (m_thisPrice == m_bid.end())
            break;
        m_thisQuote = m_thisPrice->begin();
        while (newquote.getSize() != 0) {
            if (m_thisQuote == m_thisPrice->end())
                break;
            if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setSize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }
        }

        bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
            globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

        if (m_thisPrice->empty()) {
            m_thisPrice = m_bid.erase(m_thisPrice);
        } else
            ++m_thisPrice;
    }
    //if(newquote.getSize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////////do cancel in ask list
void Stock::doCancelAsk(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_ask.begin();
    while (m_thisPrice != m_ask.end()) {
        if (m_thisPrice->getPrice() < newquote.getPrice()) {
            ++m_thisPrice;
        } else
            break;
    }
    if (m_thisPrice == m_ask.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPrice->getPrice() == newquote.getPrice()) {
        m_thisQuote = m_thisPrice->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPrice->end()) {
            if (m_thisQuote->getOrderID() == newquote.getOrderID()) {
                int size1 = m_thisQuote->getSize() - newquote.getSize();
                execute(size1, newquote, '4', destination1);

                bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                    globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

                if (size1 <= 0)
                    m_thisQuote = m_thisPrice->erase(m_thisQuote);
                if (m_thisPrice->empty()) {
                    m_ask.erase(m_thisPrice);
                    undone = 0;
                } else if (m_thisQuote != m_thisPrice->begin())
                    --m_thisQuote;
                break;
            } else {
                m_thisQuote++;
            }
        }
        if (undone) {
            if (m_thisQuote == m_thisPrice->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
    //level();
}

//////////////////////////////////////////////////////////////////////////do cancel bid
void Stock::doCancelBid(Quote& newquote, std::string destination1)
{
    m_thisPrice = m_bid.begin();
    while (m_thisPrice != m_bid.end()) {
        if (m_thisPrice->getPrice() > newquote.getPrice()) {
            ++m_thisPrice;
        } else
            break;
    }
    if (m_thisPrice == m_bid.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPrice->getPrice() == newquote.getPrice()) {
        m_thisQuote = m_thisPrice->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPrice->end()) {
            if (m_thisQuote->getOrderID() == newquote.getOrderID()) {
                int size1 = m_thisQuote->getSize() - newquote.getSize();
                execute(size1, newquote, '4', destination1);

                bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                    globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

                if (size1 <= 0)
                    m_thisQuote = m_thisPrice->erase(m_thisQuote);
                if (m_thisPrice->empty()) {
                    m_bid.erase(m_thisPrice);
                    undone = 0;
                } else if (m_thisQuote != m_thisPrice->begin())
                    --m_thisQuote;
                break;
            } else {
                m_thisQuote++;
            }
        }
        if (undone) {
            if (m_thisQuote == m_thisPrice->end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
    //level();
}

void Stock::bidInsert(Quote newquote)
{
    bookUpdate('b', m_name, newquote.getPrice(), newquote.getSize(),
        globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisPrice = m_bid.begin();

    if (!m_bid.empty()) {

        /*double temp = m_thisPrice->getPrice();
		m_thisPrice++;*/

        while (m_thisPrice != m_bid.end() && m_thisPrice->getPrice() >= newquote.getPrice()) {
            if (m_thisPrice->getPrice() == newquote.getPrice()) {
                m_thisPrice->push_back(newquote);
                m_thisPrice->setSize(m_thisPrice->getSize() + newquote.getSize());

                //bookupdate('b',name, m_thisPrice->getPrice(), m_thisPrice->getSize(), now_long()); //update the new price for broadcasting

                break;
            }
            m_thisPrice++;
        }

        if (m_thisPrice == m_bid.end()) {
            Price newprice;
            newprice.setPrice(newquote.getPrice());
            newprice.setSize(newquote.getSize());

            newprice.push_front(newquote);
            m_bid.push_back(newprice);
        } else if (m_thisPrice->getPrice() < newquote.getPrice()) {
            Price newprice;
            newprice.setPrice(newquote.getPrice());
            newprice.setSize(newquote.getSize());

            newprice.push_front(newquote);
            m_bid.insert(m_thisPrice, newprice);
        }

    } else {
        Price newprice;
        newprice.setPrice(newquote.getPrice());
        newprice.setSize(newquote.getSize());

        newprice.push_front(newquote);
        m_bid.push_back(newprice);
    }
    //level();
}

void Stock::askInsert(Quote newquote)
{
    bookUpdate('a', m_name, newquote.getPrice(), newquote.getSize(),
        globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisPrice = m_ask.begin();

    if (!m_ask.empty()) {

        /*double temp = m_thisPrice->getPrice();
		m_thisPrice++;*/

        while (m_thisPrice != m_ask.end() && m_thisPrice->getPrice() <= newquote.getPrice()) {
            if (m_thisPrice->getPrice() == newquote.getPrice()) {
                m_thisPrice->push_back(newquote);
                m_thisPrice->setSize(m_thisPrice->getSize() + newquote.getSize());
                break;
            }
            m_thisPrice++;
        }

        if (m_thisPrice == m_ask.end()) {
            Price newprice;
            newprice.setPrice(newquote.getPrice());
            newprice.setSize(newquote.getSize());

            newprice.push_front(newquote);
            m_ask.push_back(newprice);
        } else if (m_thisPrice->getPrice() > newquote.getPrice()) {
            Price newprice;
            newprice.setPrice(newquote.getPrice());
            newprice.setSize(newquote.getSize());

            newprice.push_front(newquote);
            m_ask.insert(m_thisPrice, newprice);
        }
    } else {
        Price newprice;
        newprice.setPrice(newquote.getPrice());
        newprice.setSize(newquote.getSize());

        newprice.push_front(newquote);
        m_ask.push_back(newprice);
    }
    //level();
}

//////////////////////////////////
//new order book update method
//////////////////////////////////
void Stock::bookUpdate(char book,
    std::string symbol,
    double price,
    int size,
    FIX::UtcTimeStamp utctime)
{
    Newbook newbook(book, symbol, price, size, utctime);
    //_newbook.store();
    orderbookupdate.push_back(newbook);
    //cout<<_newbook.getsymbol()<<"\t"<<_newbook.getPrice()<<"\t"<<_newbook.getSize()<<endl;
}

void Stock::bookUpdate(char book,
    std::string symbol,
    double price,
    int size,
    std::string destination,
    FIX::UtcTimeStamp time)
{
    Newbook newbook(book, symbol, price, size, destination, time);
    //_newbook.store();
    orderbookupdate.push_back(newbook);
    //cout<<_newbook.getsymbol()<<"\t"<<_newbook.getPrice()<<"\t"<<_newbook.getSize()<<endl;
}

void Stock::level()
{
    long simulationmilliseconds = globalTimeSetting.pastMilli(true);

    std::string newlevel;
    std::stringstream ss;

    ss << simulationmilliseconds << ";"
       << "Local: "
       << ";";
    //std::cout<<"Local Ask :\t"<<name<<endl;
    unsigned int depth = m_ask.size();
    depth = m_depth > depth ? depth : m_depth;
    m_thisPrice = m_ask.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPrice->getPrice() << "," << m_thisPrice->getSize() << ",";
        //std::cout<<m_thisPrice->getPrice()<<" , "<<m_thisPrice->getSize()<<" ; ";
        m_thisPrice++;
    }
    ss << ";";
    //std::cout<<endl;
    depth = m_bid.size();
    depth = m_depth > depth ? depth : m_depth;
    //std::cout<<"Local Bid :\t"<<name<<endl;
    m_thisPrice = m_bid.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisPrice->getPrice() << "," << m_thisPrice->getSize() << ",";
        //std::cout<<m_thisPrice->getPrice()<<" , "<<m_thisPrice->getSize()<<" ; ";
        m_thisPrice++;
    }
    ss << ";";
    //std::cout<<endl;
    newlevel = ss.str();
    //levels.push_back(newlevel);
    //std::cout<<newlevel<<'\n';
    //if(!actions.empty()){cout<<"  action not empty"<<"\n";} else cout<<"  empty"<<"\n";
}

void Stock::showGlobal()
{
    long simulationmilliseconds = globalTimeSetting.pastMilli(true);

    std::string newlevel;
    std::stringstream ss;

    unsigned int depth = m_globalAsk.size();
    depth = m_depth > depth ? depth : m_depth;

    ss << simulationmilliseconds << ";"
       << "Global: "
       << ";";
    //std::cout<<"Global Ask :\t"<<name<<endl;
    m_thisGlobal = m_globalAsk.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobal->getPrice() << "," << m_thisGlobal->getSize() << "," << m_thisGlobal->getDestination() << ",";
        //std::cout<<m_thisGlobal->price<<" , "<<m_thisGlobal->size<<" , "<<m_thisGlobal->destination<<" ; ";
        m_thisGlobal++;
    }
    ss << ";";
    //std::cout<<endl;
    //std::cout<<"Global Bid :\t"<<name<<endl;

    depth = m_globalBid.size();
    depth = m_depth > depth ? depth : m_depth;
    m_thisGlobal = m_globalBid.begin();
    for (unsigned int i = 0; i < depth; i++) {
        ss << m_thisGlobal->getPrice() << "," << m_thisGlobal->getSize() << "," << m_thisGlobal->getDestination() << ",";
        //std::cout<<m_thisGlobal->price<<" , "<<m_thisGlobal->size<<" , "<<m_thisGlobal->destination<<" ; ";
        m_thisGlobal++;
    }
    ss << ";";
    //std::cout<<endl;
    newlevel = ss.str();
    //levels.push_back(newlevel);
    //std::cout<<newlevel<<'\n';
}

void Stock::updateGlobalAsk(Quote newglobal)
{
    //globalprice newglobal(newquote.getstockname(), newquote.getPrice(), newquote.getSize(), newquote.getdestination(), newquote.gettime());
    bookUpdate('A', m_name, newglobal.getPrice(), newglobal.getSize(),
        newglobal.getDestination(), globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisGlobal = m_globalAsk.begin();
    while (!m_globalAsk.empty()) //&&(m_thisGlobal->price>=newglobal.price))
    {
        if (m_thisGlobal == m_globalAsk.end()) {
            m_globalAsk.push_back(newglobal);
            break;
        } else if (m_thisGlobal->getPrice() < newglobal.getPrice()) {
            /*if(m_thisGlobal->destination==newglobal.destination)
			{
				list<Quote>::iterator it;
				it=m_thisGlobal;
				++m_thisGlobal;
				m_globalAsk.erase(it);
				continue;
			}
			else m_thisGlobal++;*/

            std::list<Quote>::iterator it;
            it = m_thisGlobal;
            ++m_thisGlobal;
            m_globalAsk.erase(it);
        } else if (m_thisGlobal->getPrice() == newglobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newglobal.getDestination()) {
                m_thisGlobal->setSize(newglobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() > newglobal.getPrice()) {
            m_globalAsk.insert(m_thisGlobal, newglobal);
            break;
        }
    }
    if (m_globalAsk.empty())
        m_globalAsk.push_back(newglobal);
    //showglobal();
}

void Stock::updateGlobalBid(Quote newglobal)
{
    bookUpdate('B', m_name, newglobal.getPrice(), newglobal.getSize(),
        newglobal.getDestination(), globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisGlobal = m_globalBid.begin();
    while (!m_globalBid.empty()) {
        if (m_thisGlobal == m_globalBid.end()) {
            m_globalBid.push_back(newglobal);
            break;
        } else if (m_thisGlobal->getPrice() > newglobal.getPrice()) {
            std::list<Quote>::iterator it;
            it = m_thisGlobal;
            ++m_thisGlobal;
            m_globalBid.erase(it);
        } else if (m_thisGlobal->getPrice() == newglobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newglobal.getDestination()) {
                m_thisGlobal->setSize(newglobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() < newglobal.getPrice()) {
            m_globalBid.insert(m_thisGlobal, newglobal);
            break;
        }
    }
    if (m_globalBid.empty())
        m_globalBid.push_back(newglobal);
    //showglobal();
}

void Stock::checkGlobalBid(Quote& newquote, std::string type)
{
    if (type == "limit") {
        while (newquote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid limit." << endl;
                break;
            } else if (m_thisGlobal->getPrice() >= newquote.getPrice()) {
                int size1 = m_thisGlobal->getSize() - newquote.getSize();
                executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());

                bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                //cout<<size1<<endl;
                if (size1 <= 0) {
                    m_globalBid.pop_front();
                }
            } else if (m_thisGlobal->getPrice() < newquote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newquote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid market." << endl;
                break;
            } else //if(m_thisGlobal->price>=newquote.price)
            {
                int size1 = m_thisGlobal->getSize() - newquote.getSize();
                executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
                bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());

                if (size1 <= 0) {
                    m_globalBid.pop_front();
                }
            }
        }
    }
    //showglobal();
}

void Stock::checkGlobalAsk(Quote& newquote, std::string type)
{
    if (type == "limit") {
        while (newquote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in checkglobalask market." << endl;
                break;
            } else if (m_thisGlobal->getPrice() <= newquote.getPrice()) {
                int size1 = m_thisGlobal->getSize() - newquote.getSize();
                executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
                bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                //cout<<size1<<endl;
                if (size1 <= 0) {
                    m_globalAsk.pop_front();
                }
            } else if (m_thisGlobal->getPrice() > newquote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newquote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in check global ask market." << endl;
                break;
            } else {
                int size1 = m_thisGlobal->getSize() - newquote.getSize();
                executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
                bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                if (size1 <= 0) {
                    m_globalAsk.pop_front();
                }
            }
        }
    }
    //showglobal();
}

// hiukin
///////////////////////////////////////////////do limit buy
void Stock::doLocalLimitBuy(Quote& newquote, std::string destination1)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0f;

    // init
    m_thisPrice = m_ask.begin();
    if (m_thisPrice != m_ask.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newquote.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPrice != m_ask.end()) {
            if (m_thisQuote == m_thisPrice->end()) {
                // if m_thisQuote reach end(), go to next m_thisPrice
                ++m_thisPrice;
                m_thisQuote = m_thisPrice->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestAsk = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalAsk.begin();
        if (m_thisGlobal != m_globalAsk.end()) {
            globalBestAsk = m_thisGlobal->getPrice();
        } else {
            cout << "Global ask book is empty in dolimitbuy." << endl;
        }

        // stop if no order avaiable
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        // stop if newquote price < best price
        double bestPrice = std::min(localBestAsk, globalBestAsk);
        if (newquote.getPrice() < bestPrice) {
            break;
        }

        // convert to market buy order
        newquote.setOrderType('3');

        // compare the price
        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }

            //update the new price for broadcasting
            bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPrice->empty()) {
                m_thisPrice = m_ask.erase(m_thisPrice);
                if (m_thisPrice != m_ask.end()) {
                    m_thisQuote = m_thisPrice->begin();
                }
            }
        } else {
            //trade with global order
            int size1 = m_thisGlobal->getSize() - newquote.getSize();
            executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
            bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size1 <= 0) {
                m_globalAsk.pop_front();
            }
        }

        // recover to limit buy order
        newquote.setOrderType('1');
    }
    //if(newquote.getSize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::doLocalLimitSell(Quote& newquote, std::string destination1)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    m_thisPrice = m_bid.begin();
    if (m_thisPrice != m_bid.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newquote.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPrice != m_bid.end()) {
            if (m_thisQuote == m_thisPrice->end()) {
                // if m_thisQuote reach end(), go to next m_thisPrice
                ++m_thisPrice;
                m_thisQuote = m_thisPrice->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestBid = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalBid.begin();
        if (m_thisGlobal != m_globalBid.end()) {
            globalBestBid = m_thisGlobal->getPrice();
        } else {
            cout << "Global bid book is empty in dolimitsell." << endl;
        }

        // stop if newquote price < best price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        // stop if newquote price > best price
        double bestPrice = std::max(localBestBid, globalBestBid);
        if (newquote.getPrice() > bestPrice) {
            break;
        }

        // convert to market order
        newquote.setOrderType('4');

        // compare the price
        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setSize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }

            //update the new price for broadcasting
            bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPrice->empty()) {
                m_thisPrice = m_bid.erase(m_thisPrice);
                if (m_thisPrice != m_bid.end()) {
                    m_thisQuote = m_thisPrice->begin();
                }
            }
        } else {
            //trade with global order
            int size1 = m_thisGlobal->getSize() - newquote.getSize();
            executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
            bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size1 <= 0) {
                m_globalBid.pop_front();
            }
        }

        // recover to limit order
        newquote.setOrderType('2');
    }
    //if(newquote.getSize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////do market buy
void Stock::doLocalMarketBuy(Quote& newquote, std::string destination1)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0f;

    // init
    m_thisPrice = m_ask.begin();
    if (m_thisPrice != m_ask.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newquote.getSize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (m_thisPrice != m_ask.end()) {
            if (m_thisQuote == m_thisPrice->end()) {
                // if m_thisQuote reach end(), go to next m_thisPrice
                ++m_thisPrice;
                m_thisQuote = m_thisPrice->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestAsk = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalAsk.begin();
        if (m_thisGlobal != m_globalAsk.end()) {
            globalBestAsk = m_thisGlobal->getPrice();
        } else {
            cout << "Global ask book is empty in domarketbuy." << endl;
        }

        // compare the price
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setSize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }

            //update the new price for broadcasting
            bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPrice->empty()) {
                m_thisPrice = m_ask.erase(m_thisPrice);
                if (m_thisPrice != m_ask.end()) {
                    m_thisQuote = m_thisPrice->begin();
                }
            }
        } else {
            //trade with global order
            int size1 = m_thisGlobal->getSize() - newquote.getSize();
            executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
            bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size1 <= 0) {
                m_globalAsk.pop_front();
            }
        }
    }
    //if(newquote.getSize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::doLocalMarketSell(Quote& newquote, std::string destination1)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    m_thisPrice = m_bid.begin();
    if (m_thisPrice != m_bid.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newquote.getSize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (m_thisPrice != m_bid.end()) {
            if (m_thisQuote == m_thisPrice->end()) {
                // if m_thisQuote reach end(), go to next m_thisPrice
                ++m_thisPrice;
                m_thisQuote = m_thisPrice->begin();
                continue;
            } else {
                if (m_thisQuote->getTraderID() == newquote.getTraderID()) {
                    // skip the quote from the same trader
                    ++m_thisQuote;
                    continue;
                } else {
                    localBestBid = m_thisQuote->getPrice();
                }
            }
        }

        // get the best global price
        m_thisGlobal = m_globalBid.begin();
        if (m_thisGlobal != m_globalBid.end()) {
            globalBestBid = m_thisGlobal->getPrice();
        } else {
            cout << "Global bid book is empty in domarketsell." << endl;
        }

        // compare the price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size1 = m_thisQuote->getSize() - newquote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setSize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getSize();
            if (size1 <= 0) {
                // remove executed quote
                m_thisQuote = m_thisPrice->erase(m_thisQuote);
            }

            //update the new price for broadcasting
            bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                globalTimeSetting.simulationTimestamp());

            // remove empty price
            if (m_thisPrice->empty()) {
                m_thisPrice = m_bid.erase(m_thisPrice);
                if (m_thisPrice != m_bid.end()) {
                    m_thisQuote = m_thisPrice->begin();
                }
            }
        } else {
            //trade with global order
            int size1 = m_thisGlobal->getSize() - newquote.getSize();
            executeGlobal(size1, newquote, '2', m_thisGlobal->getDestination());
            bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size1 <= 0) {
                m_globalBid.pop_front();
            }
        }
    }
    //if(newquote.getSize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}
