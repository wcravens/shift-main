#include "threadFunction.h"

#include "globalvariables.h"

#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Stock.h"

#include <atomic>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

extern std::atomic<bool> timeout;
extern std::map<std::string, Stock> stocklist;

void debug_mode(std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed)
{
    cout << "Debug mode selected..." << endl;
    symbols.push_back("JPM");
    symbols.push_back("IBM");
}

bool fileConfig_mode(std::string file_address, std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed)
{
    std::ifstream fin(file_address, std::ios_base::in);
    if (fin.is_open()) {
        std::string line;
        while (std::getline(fin, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (key[0] == '#')
                    continue;
                if (std::getline(is_line, value)) {
                    if (key == "stock")
                        symbols.push_back(value);
                    else if (key == "date")
                        date = value;
                    else if (key == "starttime")
                        stime = value;
                    else if (key == "endtime")
                        etime = value;
                    else if (key == "speed")
                        experiment_speed = stod(value);
                    cout << value << endl;
                }
            }
        }
        fin.close();
    } else {
        cout << "Cannot open config.txt." << endl;
        return false;
    }
    return true;
}

void inputConfig_mode(std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed)
{
    cout << "Please Input experiment speed (0.5 is half speed, 1 is real time speed, 2 is double speed): " << endl;
    cin >> experiment_speed;

    cout << "Please Input Trading Date(format:yyyy-mm-dd, e.g 2014-10-08) :" << endl;
    cin >> date;

    cout << "Please Input Trading Start Time(format:hh:mm:ss, from 09:30:00 to 15:59:00):" << endl;
    cin >> stime;

    cout << "Please Input Trading End Time(format:hh:mm:ss, from 15:59:00 to 16:00:00) :" << endl;
    cin >> etime;

    int stocknums = 1; //record how many stock input
    while (1) {
        cout << endl
             << "Please Input No." << stocknums << " Stockname:" << endl;
        cout << "Refer stockname to wesite: http://www.reuters.com/finance/stocks/lookup.";
        cout << endl
             << "(To stop input, please type '0') :" << endl;
        std::string symbol;
        cin >> symbol;
        //exit the stock name input by entering 0;
        if (symbol == "0")
            break;
        symbols.push_back(symbol);
        ++stocknums;
    }
}

//function to start one stock exchange matching, for exchange thread
void createStockMarket(std::string symbol)
{
    std::map<std::string, Stock>::iterator stock;
    stock = stocklist.find(symbol);
    std::string askexchange = ""; //for modify global book update
    std::string bidexchange = ""; //for modify global book update
    double askprice = 0; //for modify global book update
    double bidprice = 0; //for modify global book update
    int asksize = 0; //for modify global book update
    int bidsize = 0; //for modify global book update

    while (!timeout) {
        //Process Clients' Order
        char type;
        Quote newquote;
        if (!stock->second.get_new_quote(newquote)) {
            std::this_thread::sleep_for(50ms);
            continue;
        } else {
            type = newquote.getordertype();

            switch (type) {
            case '1': //limit buy
            {
                stock->second.doLocalLimitBuy(newquote, "SERVER");
                //if(newquote.getsize()!=0) stock->second.checkglobalask(newquote,"limit");
                if (newquote.getsize() != 0) {
                    stock->second.bid_insert(newquote);
                    cout << "insert bid" << endl;
                }
                stock->second.level();
                //stock->second.showglobal();
                break;
            }
            case '2': //limit sell
            {
                stock->second.doLocalLimitSell(newquote, "SERVER");
                //if(newquote.getsize()!=0) stock->second.checkglobalbid(newquote,"limit");
                if (newquote.getsize() != 0) {
                    stock->second.ask_insert(newquote);
                    cout << "insert ask" << endl;
                }
                stock->second.level();
                //stock->second.showglobal();
                break;
            }
            case '3': //market buy
            {
                stock->second.doLocalMarketBuy(newquote, "SERVER");
                //if(newquote.getsize()!=0) stock->second.checkglobalask(newquote,"market");
                if (newquote.getsize() != 0)
                    stock->second.bid_insert(newquote);
                stock->second.level();
                //stock->second.showglobal();
                break;
            }
            case '4': //market sell
            {
                stock->second.doLocalMarketSell(newquote, "SERVER");
                //if(newquote.getsize()!=0) stock->second.checkglobalbid(newquote,"market");
                if (newquote.getsize() != 0)
                    stock->second.ask_insert(newquote);
                stock->second.level();
                //stock->second.showglobal();
                break;
            }
            case '5': //cancel bid
            {
                stock->second.docancelbid(newquote, "SERVER");
                stock->second.level();
                //stock->second.showglobal();
                std::cout << "done cancel bid" << endl;
                break;
            }
            case '6': //cancel ask
            {
                stock->second.docancelask(newquote, "SERVER");
                stock->second.level();
                //stock->second.showglobal();
                std::cout << "done cancel ask" << endl;
                break;
            }
            case '7': //trade from TR
            {
                //cout<<"Global trade update: "<<newquote.getstockname()<<endl;
                stock->second.dolimitbuy(newquote, "SERVER");
                stock->second.dolimitsell(newquote, "SERVER");
                stock->second.level();
                //stock->second.showglobal();
                if (newquote.getsize() != 0) {
                    //action(string stock1,double price1,int size1,string trader_id11,string trader_id21,char order_type11,char order_type21,string order_id11,
                    //string order_id21,string time0,string time11,string time21,char decision1,string destination1);
                    //decision 4 means this is a trade update from TRTH
                    // action newaction(newquote.getstockname(), newquote.getprice(), newquote.getsize(), "T1", "T2", '1', '2', "o1", "o2", newquote.gettime(), "t2", "t3", '4', "TRTH", newquote.getutctime(), utc_now, utc_now);

                    auto utc_now = timepara.utc_now();
                    action newaction(
                        newquote.getstockname(),
                        newquote.getprice(),
                        newquote.getsize(),
                        "T1",
                        "T2",
                        '1',
                        '2',
                        "o1",
                        "o2",
                        '4',
                        "TRTH",
                        newquote.gettime(),
                        utc_now,
                        utc_now);
                    stock->second.actions.push_back(newaction);
                }
                break;
            }
            case '8': //ask update from TR
            {
                //cout<<"Global ask update: "<<newquote.getstockname()<<endl;
                if (newquote.getdestination() == askexchange) {
                    if (newquote.getprice() == askprice && newquote.getsize() == asksize) {
                        break;
                    }
                }

                askexchange = newquote.getdestination();
                askprice = newquote.getprice();
                asksize = newquote.getsize();
                stock->second.dolimitsell(newquote, "SERVER");
                if (newquote.getsize() != 0)
                    stock->second.updateglobalask(newquote);
                stock->second.level();
                //stock->second.showglobal();

                break;
            }
            case '9': // bid update from TR
            {
                if (newquote.getdestination() == bidexchange) {
                    if (newquote.getprice() == bidprice && newquote.getsize() == bidsize) {
                        break;
                    }
                }

                bidexchange = newquote.getdestination();
                bidprice = newquote.getprice();
                bidsize = newquote.getsize();
                stock->second.dolimitbuy(newquote, "SERVER");
                if (newquote.getsize() != 0)
                    stock->second.updateglobalbid(newquote);
                stock->second.level();
                //stock->second.showglobal();

                break;
            }
            }

            for_each(stock->second.actions.begin(), stock->second.actions.end(), [](action& trade) {
                //cout<<"Trade:    ";
                // trade.show();
                FIXInitiator::SendActionRecord(trade); ////send execution report to database
                FIXAcceptor::SendExecution(trade);
                //cout<<"sending execution report"<<endl;
            });
            stock->second.actions.clear();

            for_each(stock->second.orderbookupdate.begin(), stock->second.orderbookupdate.end(),
                [](Newbook& _newbook) { FIXAcceptor::SendOrderbookUpdate2All(_newbook); });
            stock->second.orderbookupdate.clear();
        }
    }
}
