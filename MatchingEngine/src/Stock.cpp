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
    long now = globalTimeSetting.pastMilli(true);

    std::lock_guard<std::mutex> g_lock(m_newGlobal_mu);
    if (!m_newGlobal.empty()) {
        quote = m_newGlobal.front();
        if (quote.getMilli() < now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> l_lock(m_newLocal_mu);
    if (!m_newLocal.empty()) {
        Quote* newLocal = &m_newLocal.front();
        if (good) {
            if (quote.getMilli() >= newLocal->getMilli()) {
                quote = *newLocal;
                m_newLocal.pop();
            } else
                m_newGlobal.pop();
        } else if (newLocal->getMilli() < now) {
            good = true;
            quote = *newLocal;
            m_newLocal.pop();
        }
    } else if (good) {
        m_newGlobal.pop();
    }

    return good;
}

void Stock::setStockName(std::string stockName)
{
    m_stockName = std::move(stockName);
}

std::string Stock::getStockName()
{
    return m_stockName;
}

///decision can be '4' means cancel or '2' means trade, record trade or cancel with object actions, size=m_thisQuote->getSize()-quote2->getSize()
void Stock::execute(int size, Quote& newQuote, char decision, std::string destination)
{
    int executeSize = std::min(m_thisQuote->getSize(), newQuote.getSize()); //////#include <algorithm>
    m_thisPrice->setSize(m_thisPrice->getSize() - executeSize);
    //int newQuotesize;

    if (size >= 0) {
        m_thisQuote->setSize(size);
        //newQuotesize=0;
        newQuote.setSize(0);
        //cout<<newQuote.getSize();

    } else {
        m_thisQuote->setSize(0);
        //newQuotesize=-size;
        newQuote.setSize(-size);
        //cout<<newQuote.getSize();
    }

    auto now = globalTimeSetting.simulationTimestamp();
    Action act(
        newQuote.getStockName(),
        m_thisQuote->getPrice(),
        executeSize,
        m_thisQuote->getTraderID(),
        newQuote.getTraderID(),
        m_thisQuote->getOrderType(),
        newQuote.getOrderType(),
        m_thisQuote->getOrderID(),
        newQuote.getOrderID(),
        decision,
        destination,
        now,
        m_thisQuote->getTime(),
        newQuote.getTime());

    actions.push_back(act);
    //level();
    // cout << "actions:  " << actions.begin()->time << actions.begin()->decision << endl;
    //return newQuotesize;
}

void Stock::executeGlobal(int size, Quote& newQuote, char decision, std::string destination)
{
    int executeSize = std::min(m_thisGlobal->getSize(), newQuote.getSize());
    if (size >= 0) {
        m_thisGlobal->setSize(size);
        newQuote.setSize(0);
    } else {
        m_thisGlobal->setSize(0);
        newQuote.setSize(-size);
    }

    auto now = globalTimeSetting.simulationTimestamp();
    Action act(
        newQuote.getStockName(),
        m_thisGlobal->getPrice(),
        executeSize,
        m_thisGlobal->getTraderID(),
        newQuote.getTraderID(),
        m_thisGlobal->getOrderType(),
        newQuote.getOrderType(),
        m_thisGlobal->getOrderID(),
        newQuote.getOrderID(),
        decision,
        destination,
        now,
        m_thisGlobal->getTime(),
        newQuote.getTime());

    actions.push_back(act);
    //level();
    //cout<<"Global Actions happening!!!!"<<endl;
}

///////////////////////////////////////////////do limit buy
void Stock::doLimitBuy(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_ask.begin();
    while (newQuote.getSize() != 0) //search list<Price> ask for best price, smaller than newQuote
    {
        if (m_thisPrice == m_ask.end())
            break;
        if (m_thisPrice->getPrice() <= newQuote.getPrice()) {
            m_thisQuote = m_thisPrice->begin();
            while (newQuote.getSize() != 0) {
                if (m_thisQuote == m_thisPrice->end())
                    break;
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
                //cout<<'\n'<<size;
                //newQuote.setSize(execute(size,newQuote,'T',destination));
                execute(size, newQuote, '2', destination);
                //cout<<newQuote.getSize();
                if (size <= 0) {
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
    //if(newQuote.getSize()!=0){bid_insert(newQuote);}  //newQuote has large size so that it has to be listed in the bid list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::doLimitSell(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_bid.begin();
    while (newQuote.getSize() != 0) //search list<Price> bid for best price, smaller than newQuote
    {
        if (m_thisPrice == m_bid.end())
            break;
        if (m_thisPrice->getPrice() >= newQuote.getPrice()) {
            m_thisQuote = m_thisPrice->begin();
            while (newQuote.getSize() != 0) {
                if (m_thisQuote == m_thisPrice->end())
                    break;
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                    ++m_thisQuote;
                    continue;
                }
                int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
                //cout<<'\n'<<size;
                //newQuote.setSize(execute(size,newQuote,'T',destination));
                execute(size, newQuote, '2', destination);
                //cout<<newQuote.getSize();
                if (size <= 0) {
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
    //if(newQuote.getSize()!=0){ask_insert(newQuote);}
    //level();
}

///////////////////////////////////////////do market buy
void Stock::doMarketBuy(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_ask.begin();
    while (newQuote.getSize() != 0) //search list<Price> ask for best price
    {
        if (m_thisPrice == m_ask.end())
            break;
        m_thisQuote = m_thisPrice->begin();
        while (newQuote.getSize() != 0) {
            if (m_thisQuote == m_thisPrice->end())
                break;
            if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size;
            //newQuote.setSize(execute(size,newQuote,'T',destination));
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
    //if(newQuote.getSize()!=0){bid_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::doMarketSell(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_bid.begin();
    while (newQuote.getSize() != 0) //search list<Price> bid for best price
    {
        if (m_thisPrice == m_bid.end())
            break;
        m_thisQuote = m_thisPrice->begin();
        while (newQuote.getSize() != 0) {
            if (m_thisQuote == m_thisPrice->end())
                break;
            if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
                ++m_thisQuote;
                continue;
            }
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size;
            //newQuote.setSize(execute(size,newQuote,'T',destination));
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
    //if(newQuote.getSize()!=0){ask_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////////do cancel in ask list
void Stock::doCancelAsk(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_ask.begin();
    while (m_thisPrice != m_ask.end()) {
        if (m_thisPrice->getPrice() < newQuote.getPrice()) {
            ++m_thisPrice;
        } else
            break;
    }
    if (m_thisPrice == m_ask.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPrice->getPrice() == newQuote.getPrice()) {
        m_thisQuote = m_thisPrice->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPrice->end()) {
            if (m_thisQuote->getOrderID() == newQuote.getOrderID()) {
                int size = m_thisQuote->getSize() - newQuote.getSize();
                execute(size, newQuote, '4', destination);

                bookUpdate('a', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                    globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

                if (size <= 0)
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
void Stock::doCancelBid(Quote& newQuote, std::string destination)
{
    m_thisPrice = m_bid.begin();
    while (m_thisPrice != m_bid.end()) {
        if (m_thisPrice->getPrice() > newQuote.getPrice()) {
            ++m_thisPrice;
        } else
            break;
    }
    if (m_thisPrice == m_bid.end())
        cout << "no such quote to be canceled.\n";
    else if (m_thisPrice->getPrice() == newQuote.getPrice()) {
        m_thisQuote = m_thisPrice->begin();
        int undone = 1;
        while (m_thisQuote != m_thisPrice->end()) {
            if (m_thisQuote->getOrderID() == newQuote.getOrderID()) {
                int size = m_thisQuote->getSize() - newQuote.getSize();
                execute(size, newQuote, '4', destination);

                bookUpdate('b', m_name, m_thisPrice->getPrice(), m_thisPrice->getSize(),
                    globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting

                if (size <= 0)
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

void Stock::bidInsert(Quote newQuote)
{
    bookUpdate('b', m_name, newQuote.getPrice(), newQuote.getSize(),
        globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisPrice = m_bid.begin();

    if (!m_bid.empty()) {

        /*double temp = m_thisPrice->getPrice();
		m_thisPrice++;*/

        while (m_thisPrice != m_bid.end() && m_thisPrice->getPrice() >= newQuote.getPrice()) {
            if (m_thisPrice->getPrice() == newQuote.getPrice()) {
                m_thisPrice->push_back(newQuote);
                m_thisPrice->setSize(m_thisPrice->getSize() + newQuote.getSize());

                //bookupdate('b',name, m_thisPrice->getPrice(), m_thisPrice->getSize(), now_long()); //update the new price for broadcasting

                break;
            }
            m_thisPrice++;
        }

        if (m_thisPrice == m_bid.end()) {
            Price newPrice;
            newPrice.setPrice(newQuote.getPrice());
            newPrice.setSize(newQuote.getSize());

            newPrice.push_front(newQuote);
            m_bid.push_back(newPrice);
        } else if (m_thisPrice->getPrice() < newQuote.getPrice()) {
            Price newPrice;
            newPrice.setPrice(newQuote.getPrice());
            newPrice.setSize(newQuote.getSize());

            newPrice.push_front(newQuote);
            m_bid.insert(m_thisPrice, newPrice);
        }

    } else {
        Price newPrice;
        newPrice.setPrice(newQuote.getPrice());
        newPrice.setSize(newQuote.getSize());

        newPrice.push_front(newQuote);
        m_bid.push_back(newPrice);
    }
    //level();
}

void Stock::askInsert(Quote newQuote)
{
    bookUpdate('a', m_name, newQuote.getPrice(), newQuote.getSize(),
        globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisPrice = m_ask.begin();

    if (!m_ask.empty()) {

        /*double temp = m_thisPrice->getPrice();
		m_thisPrice++;*/

        while (m_thisPrice != m_ask.end() && m_thisPrice->getPrice() <= newQuote.getPrice()) {
            if (m_thisPrice->getPrice() == newQuote.getPrice()) {
                m_thisPrice->push_back(newQuote);
                m_thisPrice->setSize(m_thisPrice->getSize() + newQuote.getSize());
                break;
            }
            m_thisPrice++;
        }

        if (m_thisPrice == m_ask.end()) {
            Price newPrice;
            newPrice.setPrice(newQuote.getPrice());
            newPrice.setSize(newQuote.getSize());

            newPrice.push_front(newQuote);
            m_ask.push_back(newPrice);
        } else if (m_thisPrice->getPrice() > newQuote.getPrice()) {
            Price newPrice;
            newPrice.setPrice(newQuote.getPrice());
            newPrice.setSize(newQuote.getSize());

            newPrice.push_front(newQuote);
            m_ask.insert(m_thisPrice, newPrice);
        }
    } else {
        Price newPrice;
        newPrice.setPrice(newQuote.getPrice());
        newPrice.setSize(newQuote.getSize());

        newPrice.push_front(newQuote);
        m_ask.push_back(newPrice);
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
    FIX::UtcTimeStamp time)
{
    Newbook newbook(book, symbol, price, size, time);
    //_newbook.store();
    orderBookUpdate.push_back(newbook);
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
    orderBookUpdate.push_back(newbook);
    //cout<<_newbook.getsymbol()<<"\t"<<_newbook.getPrice()<<"\t"<<_newbook.getSize()<<endl;
}

void Stock::level()
{
    long simulationMilli = globalTimeSetting.pastMilli(true);

    std::string newLevel;
    std::stringstream ss;

    ss << simulationMilli << ";"
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
    newLevel = ss.str();
    //levels.push_back(newLevel);
    //std::cout<<newLevel<<'\n';
    //if(!actions.empty()){cout<<"  action not empty"<<"\n";} else cout<<"  empty"<<"\n";
}

void Stock::showGlobal()
{
    long simulationMilli = globalTimeSetting.pastMilli(true);

    std::string newLevel;
    std::stringstream ss;

    unsigned int depth = m_globalAsk.size();
    depth = m_depth > depth ? depth : m_depth;

    ss << simulationMilli << ";"
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
    newLevel = ss.str();
    //levels.push_back(newLevel);
    //std::cout<<newLevel<<'\n';
}

void Stock::updateGlobalAsk(Quote newGlobal)
{
    //globalprice newGlobal(newQuote.getstockName(), newQuote.getPrice(), newQuote.getSize(), newQuote.getDestination(), newQuote.getTime());
    bookUpdate('A', m_name, newGlobal.getPrice(), newGlobal.getSize(),
        newGlobal.getDestination(), globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisGlobal = m_globalAsk.begin();
    while (!m_globalAsk.empty()) //&&(m_thisGlobal->price>=newGlobal.price))
    {
        if (m_thisGlobal == m_globalAsk.end()) {
            m_globalAsk.push_back(newGlobal);
            break;
        } else if (m_thisGlobal->getPrice() < newGlobal.getPrice()) {
            /*if(m_thisGlobal->destination==newGlobal.destination)
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
        } else if (m_thisGlobal->getPrice() == newGlobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newGlobal.getDestination()) {
                m_thisGlobal->setSize(newGlobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() > newGlobal.getPrice()) {
            m_globalAsk.insert(m_thisGlobal, newGlobal);
            break;
        }
    }
    if (m_globalAsk.empty())
        m_globalAsk.push_back(newGlobal);
    //showglobal();
}

void Stock::updateGlobalBid(Quote newGlobal)
{
    bookUpdate('B', m_name, newGlobal.getPrice(), newGlobal.getSize(),
        newGlobal.getDestination(), globalTimeSetting.simulationTimestamp()); //update the new price for broadcasting
    m_thisGlobal = m_globalBid.begin();
    while (!m_globalBid.empty()) {
        if (m_thisGlobal == m_globalBid.end()) {
            m_globalBid.push_back(newGlobal);
            break;
        } else if (m_thisGlobal->getPrice() > newGlobal.getPrice()) {
            std::list<Quote>::iterator it;
            it = m_thisGlobal;
            ++m_thisGlobal;
            m_globalBid.erase(it);
        } else if (m_thisGlobal->getPrice() == newGlobal.getPrice()) {
            if (m_thisGlobal->getDestination() == newGlobal.getDestination()) {
                m_thisGlobal->setSize(newGlobal.getSize());
                break;
            } else
                m_thisGlobal++;
        } else if (m_thisGlobal->getPrice() < newGlobal.getPrice()) {
            m_globalBid.insert(m_thisGlobal, newGlobal);
            break;
        }
    }
    if (m_globalBid.empty())
        m_globalBid.push_back(newGlobal);
    //showglobal();
}

void Stock::checkGlobalBid(Quote& newQuote, std::string type)
{
    if (type == "limit") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid limit." << endl;
                break;
            } else if (m_thisGlobal->getPrice() >= newQuote.getPrice()) {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());

                bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                //cout<<size<<endl;
                if (size <= 0) {
                    m_globalBid.pop_front();
                }
            } else if (m_thisGlobal->getPrice() < newQuote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalBid.begin();
            if (m_globalBid.empty()) {
                cout << "Global bid book is empty in checkglobalbid market." << endl;
                break;
            } else //if(m_thisGlobal->price>=newQuote.price)
            {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());

                if (size <= 0) {
                    m_globalBid.pop_front();
                }
            }
        }
    }
    //showglobal();
}

void Stock::checkGlobalAsk(Quote& newQuote, std::string type)
{
    if (type == "limit") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in checkglobalask market." << endl;
                break;
            } else if (m_thisGlobal->getPrice() <= newQuote.getPrice()) {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                //cout<<size<<endl;
                if (size <= 0) {
                    m_globalAsk.pop_front();
                }
            } else if (m_thisGlobal->getPrice() > newQuote.getPrice())
                break;
        }
    } else if (type == "market") {
        while (newQuote.getSize() != 0) {
            m_thisGlobal = m_globalAsk.begin();
            if (m_globalAsk.empty()) {
                cout << "Global ask book is empty in check global ask market." << endl;
                break;
            } else {
                int size = m_thisGlobal->getSize() - newQuote.getSize();
                executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
                bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                    m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
                if (size <= 0) {
                    m_globalAsk.pop_front();
                }
            }
        }
    }
    //showglobal();
}

// hiukin
///////////////////////////////////////////////do limit buy
void Stock::doLocalLimitBuy(Quote& newQuote, std::string destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0f;

    // init
    m_thisPrice = m_ask.begin();
    if (m_thisPrice != m_ask.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newQuote.getSize() != 0) {

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
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
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

        // stop if newQuote price < best price
        double bestPrice = std::min(localBestAsk, globalBestAsk);
        if (newQuote.getPrice() < bestPrice) {
            break;
        }

        // convert to market buy order
        newQuote.setOrderType('3');

        // compare the price
        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsk.pop_front();
            }
        }

        // recover to limit buy order
        newQuote.setOrderType('1');
    }
    //if(newQuote.getSize()!=0){bid_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::doLocalLimitSell(Quote& newQuote, std::string destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    m_thisPrice = m_bid.begin();
    if (m_thisPrice != m_bid.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newQuote.getSize() != 0) {

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
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
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

        // stop if newQuote price < best price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        // stop if newQuote price > best price
        double bestPrice = std::max(localBestBid, globalBestBid);
        if (newQuote.getPrice() > bestPrice) {
            break;
        }

        // convert to market order
        newQuote.setOrderType('4');

        // compare the price
        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size;
            //newQuote.setSize(execute(size,newQuote,'T',destination));
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBid.pop_front();
            }
        }

        // recover to limit order
        newQuote.setOrderType('2');
    }
    //if(newQuote.getSize()!=0){ask_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////do market buy
void Stock::doLocalMarketBuy(Quote& newQuote, std::string destination)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0f;

    // init
    m_thisPrice = m_ask.begin();
    if (m_thisPrice != m_ask.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newQuote.getSize() != 0) {

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
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
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
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size;
            //newQuote.setSize(execute(size,newQuote,'T',destination));
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate('A', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalAsk.pop_front();
            }
        }
    }
    //if(newQuote.getSize()!=0){bid_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::doLocalMarketSell(Quote& newQuote, std::string destination)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    m_thisPrice = m_bid.begin();
    if (m_thisPrice != m_bid.end()) {
        m_thisQuote = m_thisPrice->begin();
    }
    while (newQuote.getSize() != 0) {

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
                if (m_thisQuote->getTraderID() == newQuote.getTraderID()) {
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
            int size = m_thisQuote->getSize() - newQuote.getSize(); //compare the two sizes.
            //cout<<'\n'<<size;
            //newQuote.setSize(execute(size,newQuote,'T',destination));
            execute(size, newQuote, '2', destination);
            //cout<<newQuote.getSize();
            if (size <= 0) {
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
            int size = m_thisGlobal->getSize() - newQuote.getSize();
            executeGlobal(size, newQuote, '2', m_thisGlobal->getDestination());
            bookUpdate('B', m_name, m_thisGlobal->getPrice(), m_thisGlobal->getSize(),
                m_thisGlobal->getDestination(), globalTimeSetting.simulationTimestamp());
            if (size <= 0) {
                m_globalBid.pop_front();
            }
        }
    }
    //if(newQuote.getSize()!=0){ask_insert(newQuote);}  //newQuote has large size so that it has to be listed in the ask list.
    //level();
}
