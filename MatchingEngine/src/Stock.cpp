#include "Stock.h"

#include "globalvariables.h"

#include <shift/miscutils/terminal/Common.h>

Stock::Stock(std::string name1)
{
    name = name1;
}
Stock::Stock(void)
{
}

Stock::~Stock(void)
{
}

void Stock::buf_new_global(Quote& quote)
{
    try {
        std::lock_guard<std::mutex> lock(new_global_mu);
        new_global.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

void Stock::buf_new_local(Quote& quote)
{
    try {
        std::lock_guard<std::mutex> lock(new_global_mu);
        new_local.push(quote);
    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

bool Stock::get_new_quote(Quote& quote)
{
    bool good = false;
    long mili_now = timepara.past_milli();

    std::lock_guard<std::mutex> g_lock(new_global_mu);
    if (!new_global.empty()) {
        quote = new_global.front();
        if (quote.getmili() < mili_now) {
            good = true;
        }
    }
    std::lock_guard<std::mutex> l_lock(new_local_mu);
    if (!new_local.empty()) {
        Quote* newlocal = &new_local.front();
        if (good) {
            if (quote.getmili() >= newlocal->getmili()) {
                quote = *newlocal;
                new_local.pop();
            } else
                new_global.pop();
        } else if (newlocal->getmili() < mili_now) {
            good = true;
            quote = *newlocal;
            new_local.pop();
        }
    } else if (good) {
        new_global.pop();
    }

    return good;
}

void Stock::setstockname(std::string name1)
{
    name = name1;
}
std::string Stock::getstockname()
{
    return name;
}

///decision1 can be 'C' means cancel or '2' means trade, record trade or cancel with object actions, size1=thisquote->getsize()-quote2->getsize()
void Stock::execute(int size1, Quote& newquote, char decision1, std::string destination1)
{
    int executesize = std::min(thisquote->getsize(), newquote.getsize()); //////#include <algorithm>
    thisprice->setsize(thisprice->getsize() - executesize);
    //int newquotesize;

    if (size1 >= 0) {
        thisquote->setsize(size1);
        //newquotesize=0;
        newquote.setsize(0);
        //cout<<newquote.getsize();

    } else {
        thisquote->setsize(0);
        //newquotesize=-size1;
        newquote.setsize(-size1);
        //cout<<newquote.getsize();
    }

    auto utc_now = timepara.utc_now();
    action act(
        newquote.getstockname(),
        thisquote->getprice(),
        executesize,
        thisquote->gettrader_id(),
        newquote.gettrader_id(),
        thisquote->getordertype(),
        newquote.getordertype(),
        thisquote->getorder_id(),
        newquote.getorder_id(),
        decision1,
        destination1,
        utc_now,
        thisquote->gettime(),
        newquote.gettime());

    actions.push_back(act);
    //level();
    // cout << "actions:  " << actions.begin()->time1 << actions.begin()->decision << endl;
    //return newquotesize;
}

void Stock::executeglobal(int size1, Quote& newquote, char decision1, std::string destination1)
{
    int executesize = std::min(thisglobal->getsize(), newquote.getsize());
    if (size1 >= 0) {
        thisglobal->setsize(size1);
        newquote.setsize(0);
    } else {
        ;
        thisglobal->setsize(0);
        newquote.setsize(-size1);
    }

    auto utc_now = timepara.utc_now();
    action act(
        newquote.getstockname(),
        thisglobal->getprice(),
        executesize,
        thisglobal->gettrader_id(),
        newquote.gettrader_id(),
        thisglobal->getordertype(),
        newquote.getordertype(),
        thisglobal->getorder_id(),
        newquote.getorder_id(),
        decision1,
        destination1,
        utc_now,
        thisglobal->gettime(),
        newquote.gettime());

    actions.push_back(act);
    //level();
    //cout<<"Global Actions happening!!!!"<<endl;
}

///////////////////////////////////////////////do limit buy
void Stock::dolimitbuy(Quote& newquote, std::string destination1)
{
    thisprice = ask.begin();
    while (newquote.getsize() != 0) //search list<Price> ask for best price, smaller than newquote
    {
        if (thisprice == ask.end())
            break;
        if (thisprice->getprice() <= newquote.getprice()) {
            thisquote = thisprice->quotes.begin();
            while (newquote.getsize() != 0) {
                if (thisquote == thisprice->quotes.end())
                    break;
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    ++thisquote;
                    continue;
                }
                int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
                //cout<<'\n'<<size1;
                //newquote.setsize(execute(size1,newquote,'T',destination1));
                execute(size1, newquote, '2', destination1);
                //cout<<newquote.getsize();
                if (size1 <= 0) {
                    thisquote = thisprice->quotes.erase(thisquote);
                }
            }
        }

        bookupdate('a', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

        if (thisprice->quotes.empty()) {
            thisprice = ask.erase(thisprice);
        } else
            ++thisprice;
    }
    //if(newquote.getsize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the bid list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::dolimitsell(Quote& newquote, std::string destination1)
{
    thisprice = bid.begin();
    while (newquote.getsize() != 0) //search list<Price> bid for best price, smaller than newquote
    {
        if (thisprice == bid.end())
            break;
        if (thisprice->getprice() >= newquote.getprice()) {
            thisquote = thisprice->quotes.begin();
            while (newquote.getsize() != 0) {
                if (thisquote == thisprice->quotes.end())
                    break;
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    ++thisquote;
                    continue;
                }
                int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
                //cout<<'\n'<<size1;
                //newquote.setsize(execute(size1,newquote,'T',destination1));
                execute(size1, newquote, '2', destination1);
                //cout<<newquote.getsize();
                if (size1 <= 0) {
                    thisquote = thisprice->quotes.erase(thisquote);
                }
            }
        }

        bookupdate('b', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

        if (thisprice->quotes.empty()) {
            thisprice = bid.erase(thisprice);
        } else
            ++thisprice;
    }
    //if(newquote.getsize()!=0){ask_insert(newquote);}
    //level();
}

///////////////////////////////////////////do market buy
void Stock::domarketbuy(Quote& newquote, std::string destination1)
{
    thisprice = ask.begin();
    while (newquote.getsize() != 0) //search list<Price> ask for best price
    {
        if (thisprice == ask.end())
            break;
        thisquote = thisprice->quotes.begin();
        while (newquote.getsize() != 0) {
            if (thisquote == thisprice->quotes.end())
                break;
            if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                ++thisquote;
                continue;
            }
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setsize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                thisquote = thisprice->quotes.erase(thisquote);
            }
        }

        bookupdate('a', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

        if (thisprice->quotes.empty()) {
            thisprice = ask.erase(thisprice);
        } else
            ++thisprice;
    }
    //if(newquote.getsize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::domarketsell(Quote& newquote, std::string destination1)
{
    thisprice = bid.begin();
    while (newquote.getsize() != 0) //search list<Price> bid for best price
    {
        if (thisprice == bid.end())
            break;
        thisquote = thisprice->quotes.begin();
        while (newquote.getsize() != 0) {
            if (thisquote == thisprice->quotes.end())
                break;
            if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                ++thisquote;
                continue;
            }
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setsize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                thisquote = thisprice->quotes.erase(thisquote);
            }
        }

        bookupdate('b', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

        if (thisprice->quotes.empty()) {
            thisprice = bid.erase(thisprice);
        } else
            ++thisprice;
    }
    //if(newquote.getsize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////////do cancel in ask list
void Stock::docancelask(Quote& newquote, std::string destination1)
{
    thisprice = ask.begin();
    while (thisprice != ask.end()) {
        if (thisprice->getprice() < newquote.getprice()) {
            ++thisprice;
        } else
            break;
    }
    if (thisprice == ask.end())
        cout << "no such quote to be canceled.\n";
    else if (thisprice->getprice() == newquote.getprice()) {
        thisquote = thisprice->quotes.begin();
        int undone = 1;
        while (thisquote != thisprice->quotes.end()) {
            if (thisquote->getorder_id() == newquote.getorder_id()) {
                int size1 = thisquote->getsize() - newquote.getsize();
                execute(size1, newquote, 'C', destination1);

                bookupdate('a', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

                if (size1 <= 0)
                    thisquote = thisprice->quotes.erase(thisquote);
                if (thisprice->quotes.empty()) {
                    ask.erase(thisprice);
                    undone = 0;
                } else if (thisquote != thisprice->quotes.begin())
                    --thisquote;
                break;
            } else {
                thisquote++;
            }
        }
        if (undone) {
            if (thisquote == thisprice->quotes.end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
    //level();
}

//////////////////////////////////////////////////////////////////////////do cancel bid
void Stock::docancelbid(Quote& newquote, std::string destination1)
{
    thisprice = bid.begin();
    while (thisprice != bid.end()) {
        if (thisprice->getprice() > newquote.getprice()) {
            ++thisprice;
        } else
            break;
    }
    if (thisprice == bid.end())
        cout << "no such quote to be canceled.\n";
    else if (thisprice->getprice() == newquote.getprice()) {
        thisquote = thisprice->quotes.begin();
        int undone = 1;
        while (thisquote != thisprice->quotes.end()) {
            if (thisquote->getorder_id() == newquote.getorder_id()) {
                int size1 = thisquote->getsize() - newquote.getsize();
                execute(size1, newquote, 'C', destination1);

                bookupdate('b', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting

                if (size1 <= 0)
                    thisquote = thisprice->quotes.erase(thisquote);
                if (thisprice->quotes.empty()) {
                    bid.erase(thisprice);
                    undone = 0;
                } else if (thisquote != thisprice->quotes.begin())
                    --thisquote;
                break;
            } else {
                thisquote++;
            }
        }
        if (undone) {
            if (thisquote == thisprice->quotes.end())
                cout << "no such quote to be canceled.\n";
        }
    } else
        cout << "no such quote to be canceled.\n";
    //level();
}

void Stock::bid_insert(Quote newquote)
{
    bookupdate('b', name, newquote.getprice(), newquote.getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting
    thisprice = bid.begin();
    //Quote newquote(it.getstockname(),it.gettrader_id(),it.getorder_id(),it.getprice(),it.getsize(),it.gettime(),it.getordertype());

    if (!bid.empty()) {

        /*double temp = thisprice->getprice();
		thisprice++;*/

        while (thisprice != bid.end() && thisprice->getprice() >= newquote.getprice()) {
            if (thisprice->getprice() == newquote.getprice()) {
                thisprice->quotes.push_back(newquote);
                thisprice->setsize(thisprice->getsize() + newquote.getsize());

                //bookupdate('b',name, thisprice->getprice(), thisprice->getsize(), now_long()); //update the new price for broadcasting

                break;
            }
            thisprice++;
        }

        if (thisprice == bid.end()) {
            Price newprice;
            newprice.setprice(newquote.getprice());
            newprice.setsize(newquote.getsize());

            newprice.quotes.push_front(newquote);
            bid.push_back(newprice);
        } else if (thisprice->getprice() < newquote.getprice()) {
            Price newprice;
            newprice.setprice(newquote.getprice());
            newprice.setsize(newquote.getsize());

            newprice.quotes.push_front(newquote);
            bid.insert(thisprice, newprice);
        }

    } else {
        Price newprice;
        newprice.setprice(newquote.getprice());
        newprice.setsize(newquote.getsize());

        newprice.quotes.push_front(newquote);
        bid.push_back(newprice);
    }
    //level();
}

void Stock::ask_insert(Quote newquote)
{
    bookupdate('a', name, newquote.getprice(), newquote.getsize(), timepara.past_milli(), timepara.utc_now()); //update the new price for broadcasting
    thisprice = ask.begin();
    //Quote newquote(it.getstockname(),it.gettrader_id(),it.getorder_id(),it.getprice(),it.getsize(),it.gettime(),it.getordertype());

    if (!ask.empty()) {

        /*double temp = thisprice->getprice();
		thisprice++;*/

        while (thisprice != ask.end() && thisprice->getprice() <= newquote.getprice()) {
            if (thisprice->getprice() == newquote.getprice()) {
                thisprice->quotes.push_back(newquote);
                thisprice->setsize(thisprice->getsize() + newquote.getsize());
                break;
            }
            thisprice++;
        }

        if (thisprice == ask.end()) {
            Price newprice;
            newprice.setprice(newquote.getprice());
            newprice.setsize(newquote.getsize());

            newprice.quotes.push_front(newquote);
            ask.push_back(newprice);
        } else if (thisprice->getprice() > newquote.getprice()) {
            Price newprice;
            newprice.setprice(newquote.getprice());
            newprice.setsize(newquote.getsize());

            newprice.quotes.push_front(newquote);
            ask.insert(thisprice, newprice);
        }
    } else {
        Price newprice;
        newprice.setprice(newquote.getprice());
        newprice.setsize(newquote.getsize());

        newprice.quotes.push_front(newquote);
        ask.push_back(newprice);
    }
    //level();
}

//////////////////////////////////
//new order book update method
//////////////////////////////////
void Stock::bookupdate(char _book, std::string _symbol, double _price, int _size, double _time, FIX::UtcTimeStamp _utctime)
{
    Newbook _newbook(_book, _symbol, _price, _size, _time, _utctime);
    //_newbook.store();
    orderbookupdate.push_back(_newbook);
    //cout<<_newbook.getsymbol()<<"\t"<<_newbook.getprice()<<"\t"<<_newbook.getsize()<<endl;
}

void Stock::bookupdate(char _book, std::string _symbol, double _price, int _size, double _time, std::string _destination, FIX::UtcTimeStamp _utctime)
{
    Newbook _newbook(_book, _symbol, _price, _size, _time, _destination, _utctime);
    //_newbook.store();
    orderbookupdate.push_back(_newbook);
    //cout<<_newbook.getsymbol()<<"\t"<<_newbook.getprice()<<"\t"<<_newbook.getsize()<<endl;
}

void Stock::level()
{
    long realmilliseconds = timepara.past_milli();

    std::string newlevel;
    std::stringstream ss;

    ss << realmilliseconds << ";"
       << "Local: "
       << ";";
    //std::cout<<"Local Ask :\t"<<name<<endl;
    unsigned int _depth = ask.size();
    _depth = depth > _depth ? _depth : depth;
    thisprice = ask.begin();
    for (unsigned int i = 0; i < _depth; i++) {
        ss << thisprice->getprice() << "," << thisprice->getsize() << ",";
        //std::cout<<thisprice->getprice()<<" , "<<thisprice->getsize()<<" ; ";
        thisprice++;
    }
    ss << ";";
    //std::cout<<endl;
    _depth = bid.size();
    _depth = depth > _depth ? _depth : depth;
    //std::cout<<"Local Bid :\t"<<name<<endl;
    thisprice = bid.begin();
    for (unsigned int i = 0; i < _depth; i++) {
        ss << thisprice->getprice() << "," << thisprice->getsize() << ",";
        //std::cout<<thisprice->getprice()<<" , "<<thisprice->getsize()<<" ; ";
        thisprice++;
    }
    ss << ";";
    //std::cout<<endl;
    newlevel = ss.str();
    //levels.push_back(newlevel);
    //std::cout<<newlevel<<'\n';
    //if(!actions.empty()){cout<<"  action not empty"<<"\n";} else cout<<"  empty"<<"\n";
}

void Stock::showglobal()
{
    long realmilliseconds = timepara.past_milli();

    std::string newlevel;
    std::stringstream ss;

    unsigned int _depth = globalask.size();
    _depth = depth > _depth ? _depth : depth;

    ss << realmilliseconds << ";"
       << "Global: "
       << ";";
    //std::cout<<"Global Ask :\t"<<name<<endl;
    thisglobal = globalask.begin();
    for (unsigned int i = 0; i < _depth; i++) {
        ss << thisglobal->price << "," << thisglobal->size << "," << thisglobal->destination << ",";
        //std::cout<<thisglobal->price<<" , "<<thisglobal->size<<" , "<<thisglobal->destination<<" ; ";
        thisglobal++;
    }
    ss << ";";
    //std::cout<<endl;
    //std::cout<<"Global Bid :\t"<<name<<endl;

    _depth = globalbid.size();
    _depth = depth > _depth ? _depth : depth;
    thisglobal = globalbid.begin();
    for (unsigned int i = 0; i < _depth; i++) {
        ss << thisglobal->price << "," << thisglobal->size << "," << thisglobal->destination << ",";
        //std::cout<<thisglobal->price<<" , "<<thisglobal->size<<" , "<<thisglobal->destination<<" ; ";
        thisglobal++;
    }
    ss << ";";
    //std::cout<<endl;
    newlevel = ss.str();
    //levels.push_back(newlevel);
    //std::cout<<newlevel<<'\n';
}

void Stock::updateglobalask(Quote newglobal)
{
    //globalprice newglobal(newquote.getstockname(), newquote.getprice(), newquote.getsize(), newquote.getdestination(), newquote.gettime());
    bookupdate('A', name, newglobal.getprice(), newglobal.getsize(), timepara.past_milli(), newglobal.getdestination(), timepara.utc_now()); //update the new price for broadcasting
    thisglobal = globalask.begin();
    while (!globalask.empty()) //&&(thisglobal->price>=newglobal.price))
    {
        if (thisglobal == globalask.end()) {
            globalask.push_back(newglobal);
            break;
        } else if (thisglobal->price < newglobal.price) {
            /*if(thisglobal->destination==newglobal.destination)
			{
				list<Quote>::iterator it;
				it=thisglobal;
				++thisglobal;
				globalask.erase(it);
				continue;
			}
			else thisglobal++;*/

            std::list<Quote>::iterator it;
            it = thisglobal;
            ++thisglobal;
            globalask.erase(it);
        } else if (thisglobal->price == newglobal.price) {
            if (thisglobal->destination == newglobal.destination) {
                thisglobal->size = newglobal.size;
                break;
            } else
                thisglobal++;
        } else if (thisglobal->price > newglobal.price) {
            globalask.insert(thisglobal, newglobal);
            break;
        }
    }
    if (globalask.empty())
        globalask.push_back(newglobal);
    //showglobal();
}

void Stock::updateglobalbid(Quote newglobal)
{
    bookupdate('B', name, newglobal.getprice(), newglobal.getsize(), timepara.past_milli(), newglobal.getdestination(), timepara.utc_now()); //update the new price for broadcasting
    thisglobal = globalbid.begin();
    while (!globalbid.empty()) {
        if (thisglobal == globalbid.end()) {
            globalbid.push_back(newglobal);
            break;
        } else if (thisglobal->price > newglobal.price) {
            std::list<Quote>::iterator it;
            it = thisglobal;
            ++thisglobal;
            globalbid.erase(it);
        } else if (thisglobal->price == newglobal.price) {
            if (thisglobal->destination == newglobal.destination) {
                thisglobal->size = newglobal.size;
                break;
            } else
                thisglobal++;
        } else if (thisglobal->price < newglobal.price) {
            globalbid.insert(thisglobal, newglobal);
            break;
        }
    }
    if (globalbid.empty())
        globalbid.push_back(newglobal);
    //showglobal();
}

void Stock::checkglobalbid(Quote& newquote, std::string type)
{
    if (type == "limit") {
        while (newquote.size != 0) {
            thisglobal = globalbid.begin();
            if (globalbid.empty()) {
                cout << "Global bid book is empty in checkglobalbid limit." << endl;
                break;
            } else if (thisglobal->price >= newquote.price) {
                int size1 = thisglobal->size - newquote.size;
                executeglobal(size1, newquote, '2', thisglobal->destination);

                bookupdate('B', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
                //cout<<size1<<endl;
                if (size1 <= 0) {
                    globalbid.pop_front();
                }
            } else if (thisglobal->price < newquote.price)
                break;
        }
    } else if (type == "market") {
        while (newquote.size != 0) {
            thisglobal = globalbid.begin();
            if (globalbid.empty()) {
                cout << "Global bid book is empty in checkglobalbid market." << endl;
                break;
            } else //if(thisglobal->price>=newquote.price)
            {
                int size1 = thisglobal->size - newquote.size;
                executeglobal(size1, newquote, '2', thisglobal->destination);
                bookupdate('B', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());

                if (size1 <= 0) {
                    globalbid.pop_front();
                }
            }
        }
    }
    //showglobal();
}

void Stock::checkglobalask(Quote& newquote, std::string type)
{
    if (type == "limit") {
        while (newquote.size != 0) {
            thisglobal = globalask.begin();
            if (globalask.empty()) {
                cout << "Global ask book is empty in checkglobalask market." << endl;
                break;
            } else if (thisglobal->price <= newquote.price) {
                int size1 = thisglobal->size - newquote.size;
                executeglobal(size1, newquote, '2', thisglobal->destination);
                bookupdate('A', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
                //cout<<size1<<endl;
                if (size1 <= 0) {
                    globalask.pop_front();
                }
            } else if (thisglobal->price > newquote.price)
                break;
        }
    } else if (type == "market") {
        while (newquote.size != 0) {
            thisglobal = globalask.begin();
            if (globalask.empty()) {
                cout << "Global ask book is empty in check global ask market." << endl;
                break;
            } else {
                int size1 = thisglobal->size - newquote.size;
                executeglobal(size1, newquote, '2', thisglobal->destination);
                bookupdate('A', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
                if (size1 <= 0) {
                    globalask.pop_front();
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
    thisprice = ask.begin();
    if (thisprice != ask.end()) {
        thisquote = thisprice->quotes.begin();
    }
    while (newquote.getsize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (thisprice != ask.end()) {
            if (thisquote == thisprice->quotes.end()) {
                // if thisquote reach end(), go to next thisprice
                ++thisprice;
                thisquote = thisprice->quotes.begin();
                continue;
            } else {
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    // skip the quote from the same trader
                    ++thisquote;
                    continue;
                } else {
                    localBestAsk = thisquote->getprice();
                }
            }
        }

        // get the best global price
        thisglobal = globalask.begin();
        if (thisglobal != globalask.end()) {
            globalBestAsk = thisglobal->getprice();
        } else {
            cout << "Global ask book is empty in dolimitbuy." << endl;
        }

        // stop if no order avaiable
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        // stop if newquote price < best price
        double bestPrice = std::min(localBestAsk, globalBestAsk);
        if (newquote.getprice() < bestPrice) {
            break;
        }

        // convert to market buy order
        newquote.setordertype('3');

        // compare the price
        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                // remove executed quote
                thisquote = thisprice->quotes.erase(thisquote);
            }

            //update the new price for broadcasting
            bookupdate('a', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now());

            // remove empty price
            if (thisprice->quotes.empty()) {
                thisprice = ask.erase(thisprice);
                if (thisprice != ask.end()) {
                    thisquote = thisprice->quotes.begin();
                }
            }
        } else {
            //trade with global order
            int size1 = thisglobal->size - newquote.size;
            executeglobal(size1, newquote, '2', thisglobal->destination);
            bookupdate('A', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
            if (size1 <= 0) {
                globalask.pop_front();
            }
        }

        // recover to limit buy order
        newquote.setordertype('1');
    }
    //if(newquote.getsize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

////////////////////////////////////////////////////////////////////////do limit sell
void Stock::doLocalLimitSell(Quote& newquote, std::string destination1)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    thisprice = bid.begin();
    if (thisprice != bid.end()) {
        thisquote = thisprice->quotes.begin();
    }
    while (newquote.getsize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (thisprice != bid.end()) {
            if (thisquote == thisprice->quotes.end()) {
                // if thisquote reach end(), go to next thisprice
                ++thisprice;
                thisquote = thisprice->quotes.begin();
                continue;
            } else {
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    // skip the quote from the same trader
                    ++thisquote;
                    continue;
                } else {
                    localBestBid = thisquote->getprice();
                }
            }
        }

        // get the best global price
        thisglobal = globalbid.begin();
        if (thisglobal != globalbid.end()) {
            globalBestBid = thisglobal->getprice();
        } else {
            cout << "Global bid book is empty in dolimitsell." << endl;
        }

        // stop if newquote price < best price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        // stop if newquote price > best price
        double bestPrice = std::max(localBestBid, globalBestBid);
        if (newquote.getprice() > bestPrice) {
            break;
        }

        // convert to market order
        newquote.setordertype('4');

        // compare the price
        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setsize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                // remove executed quote
                thisquote = thisprice->quotes.erase(thisquote);
            }

            //update the new price for broadcasting
            bookupdate('b', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now());

            // remove empty price
            if (thisprice->quotes.empty()) {
                thisprice = bid.erase(thisprice);
                if (thisprice != bid.end()) {
                    thisquote = thisprice->quotes.begin();
                }
            }
        } else {
            //trade with global order
            int size1 = thisglobal->size - newquote.size;
            executeglobal(size1, newquote, '2', thisglobal->destination);
            bookupdate('B', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
            if (size1 <= 0) {
                globalbid.pop_front();
            }
        }

        // recover to limit order
        newquote.setordertype('2');
    }
    //if(newquote.getsize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

///////////////////////////////////////////do market buy
void Stock::doLocalMarketBuy(Quote& newquote, std::string destination1)
{
    double localBestAsk, globalBestAsk;
    double maxAskPrice = 10000.0f;

    // init
    thisprice = ask.begin();
    if (thisprice != ask.end()) {
        thisquote = thisprice->quotes.begin();
    }
    while (newquote.getsize() != 0) {

        // init variables
        localBestAsk = maxAskPrice;
        globalBestAsk = maxAskPrice;

        // get the best local price
        if (thisprice != ask.end()) {
            if (thisquote == thisprice->quotes.end()) {
                // if thisquote reach end(), go to next thisprice
                ++thisprice;
                thisquote = thisprice->quotes.begin();
                continue;
            } else {
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    // skip the quote from the same trader
                    ++thisquote;
                    continue;
                } else {
                    localBestAsk = thisquote->getprice();
                }
            }
        }

        // get the best global price
        thisglobal = globalask.begin();
        if (thisglobal != globalask.end()) {
            globalBestAsk = thisglobal->getprice();
        } else {
            cout << "Global ask book is empty in domarketbuy." << endl;
        }

        // compare the price
        if (localBestAsk == maxAskPrice && globalBestAsk == maxAskPrice) {
            break;
        }

        if (localBestAsk <= globalBestAsk) {
            // trade with local order
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setsize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                // remove executed quote
                thisquote = thisprice->quotes.erase(thisquote);
            }

            //update the new price for broadcasting
            bookupdate('a', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now());

            // remove empty price
            if (thisprice->quotes.empty()) {
                thisprice = ask.erase(thisprice);
                if (thisprice != ask.end()) {
                    thisquote = thisprice->quotes.begin();
                }
            }
        } else {
            //trade with global order
            int size1 = thisglobal->size - newquote.size;
            executeglobal(size1, newquote, '2', thisglobal->destination);
            bookupdate('A', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
            if (size1 <= 0) {
                globalask.pop_front();
            }
        }
    }
    //if(newquote.getsize()!=0){bid_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}

/////////////////////////////////////////////////////////////////////////do market sell
void Stock::doLocalMarketSell(Quote& newquote, std::string destination1)
{
    double localBestBid, globalBestBid;
    double minBidPrice = 0.0f;

    // init
    thisprice = bid.begin();
    if (thisprice != bid.end()) {
        thisquote = thisprice->quotes.begin();
    }
    while (newquote.getsize() != 0) {

        // init variables
        localBestBid = minBidPrice;
        globalBestBid = minBidPrice;

        // get the best local price
        if (thisprice != bid.end()) {
            if (thisquote == thisprice->quotes.end()) {
                // if thisquote reach end(), go to next thisprice
                ++thisprice;
                thisquote = thisprice->quotes.begin();
                continue;
            } else {
                if (thisquote->gettrader_id() == newquote.gettrader_id()) {
                    // skip the quote from the same trader
                    ++thisquote;
                    continue;
                } else {
                    localBestBid = thisquote->getprice();
                }
            }
        }

        // get the best global price
        thisglobal = globalbid.begin();
        if (thisglobal != globalbid.end()) {
            globalBestBid = thisglobal->getprice();
        } else {
            cout << "Global bid book is empty in domarketsell." << endl;
        }

        // compare the price
        if (localBestBid == minBidPrice && globalBestBid == minBidPrice) {
            break;
        }

        if (localBestBid >= globalBestBid) {
            // trade with local order
            int size1 = thisquote->getsize() - newquote.getsize(); //compare the two sizes.
            //cout<<'\n'<<size1;
            //newquote.setsize(execute(size1,newquote,'T',destination1));
            execute(size1, newquote, '2', destination1);
            //cout<<newquote.getsize();
            if (size1 <= 0) {
                // remove executed quote
                thisquote = thisprice->quotes.erase(thisquote);
            }

            //update the new price for broadcasting
            bookupdate('b', name, thisprice->getprice(), thisprice->getsize(), timepara.past_milli(), timepara.utc_now());

            // remove empty price
            if (thisprice->quotes.empty()) {
                thisprice = bid.erase(thisprice);
                if (thisprice != bid.end()) {
                    thisquote = thisprice->quotes.begin();
                }
            }
        } else {
            //trade with global order
            int size1 = thisglobal->size - newquote.size;
            executeglobal(size1, newquote, '2', thisglobal->destination);
            bookupdate('B', name, thisglobal->getprice(), thisglobal->getsize(), timepara.past_milli(), thisglobal->getdestination(), timepara.utc_now());
            if (size1 <= 0) {
                globalbid.pop_front();
            }
        }
    }
    //if(newquote.getsize()!=0){ask_insert(newquote);}  //newquote has large size so that it has to be listed in the ask list.
    //level();
}
