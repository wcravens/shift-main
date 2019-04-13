#include "threadFunction.h"

#include "TimeSetting.h"

#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "Stock.h"

#include <atomic>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

extern std::atomic<bool> timeout;
extern std::map<std::string, Stock> stockList;

bool fileConfigMode(std::string fileAddr, std::string& date, std::string& stime, std::string& etime, int& experimentSpeed, std::vector<std::string>& symbols)
{
    std::ifstream fin(fileAddr, std::ios_base::in);
    if (fin.is_open()) {
        std::string line;
        while (std::getline(fin, line)) {
            std::istringstream inStreamLine(line);
            std::string key;
            if (std::getline(inStreamLine, key, '=')) {
                std::string value;
                if (key[0] == '#')
                    continue;
                if (std::getline(inStreamLine, value)) {
                    if (key == "stock")
                        symbols.push_back(value);
                    else if (key == "date")
                        date = value;
                    else if (key == "starttime")
                        stime = value;
                    else if (key == "endtime")
                        etime = value;
                    else if (key == "speed")
                        experimentSpeed = stoi(value);
                    cout << value << endl;
                }
            }
        }
        fin.close();
    } else {
        cout << "Cannot open " << fileAddr << '.' << endl;
        return false;
    }
    return true;
}

void inputConfigMode(std::string& date, std::string& stime, std::string& etime, int& experimentSpeed, std::vector<std::string>& symbols)
{
    cout << "Please input simulation date (format: yyyy-mm-dd, e.g 2018-12-17):" << endl;
    cin >> date;
    cout << endl;

    cout << "Please input start time (format: hh:mm:ss, from 09:30:00 to 15:59:00):" << endl;
    cin >> stime;
    cout << endl;

    cout << "Please input end time (format:hh:mm:ss, from 15:59:00 to 16:00:00):" << endl;
    cin >> etime;
    cout << endl;

    cout << "Please input experiment speed (1 is real time speed, 2 is double speed):" << endl;
    cin >> experimentSpeed;
    cout << endl;

    cout << "Please refer to http://www.reuters.com/finance/stocks/lookup for stock tickers." << endl;

    int numStocks = 1; // record the number of inputted stocks
    while (1) {
        cout << endl
             << "Please input stock number " << numStocks << " (to stop input, please type '0'):" << endl;
        std::string symbol;
        cin >> symbol;
        // exit when input is "0":
        if (symbol == "0")
            break;
        symbols.push_back(symbol);
        ++numStocks;
    }
}

// Function to start one stock matching engine, for exchange thread
void createStockMarket(std::string symbol)
{
    std::map<std::string, Stock>::iterator stock;
    stock = stockList.find(symbol);
    std::string askExchange = ""; // for modify global book update
    std::string bidExchange = ""; // for modify global book update
    double askPrice = 0; // for modify global book update
    double bidPrice = 0; // for modify global book update
    int askSize = 0; // for modify global book update
    int bidSize = 0; // for modify global book update

    while (!timeout) { // process orders
        Order newOrder;

        if (!stock->second.getNextOrder(newOrder)) {
            std::this_thread::sleep_for(50ms);
            continue;
        } else {
            switch (newOrder.getType()) {
            case Order::Type::LIMIT_BUY: {
                stock->second.doLocalLimitBuy(newOrder, "SHIFT");
                if (newOrder.getSize() != 0) {
                    stock->second.insertLocalBid(newOrder);
                    cout << "Insert Bid" << endl;
                }
                break;
            }
            case Order::Type::LIMIT_SELL: {
                stock->second.doLocalLimitSell(newOrder, "SHIFT");
                if (newOrder.getSize() != 0) {
                    stock->second.insertLocalAsk(newOrder);
                    cout << "Insert Ask" << endl;
                }
                break;
            }
            case Order::Type::MARKET_BUY: {
                stock->second.doLocalMarketBuy(newOrder, "SHIFT");
                if (newOrder.getSize() != 0)
                    stock->second.insertLocalBid(newOrder);
                break;
            }
            case Order::Type::MARKET_SELL: {
                stock->second.doLocalMarketSell(newOrder, "SHIFT");
                if (newOrder.getSize() != 0)
                    stock->second.insertLocalAsk(newOrder);
                break;
            }
            case Order::Type::CANCEL_BID: {
                stock->second.doCancelBid(newOrder, "SHIFT");
                std::cout << "Cancel Bid Done" << endl;
                break;
            }
            case Order::Type::CANCEL_ASK: {
                stock->second.doCancelAsk(newOrder, "SHIFT");
                std::cout << "Cancel Ask Done" << endl;
                break;
            }
            case Order::Type::TRTH_TRADE: {
                stock->second.doLimitBuy(newOrder, "SHIFT");
                stock->second.doLimitSell(newOrder, "SHIFT");
                if (newOrder.getSize() != 0) {
                    auto now = globalTimeSetting.simulationTimestamp();
                    stock->second.executionReports.push_back({ newOrder.getSymbol(),
                        newOrder.getPrice(),
                        newOrder.getSize(),
                        "T1",
                        "T2",
                        Order::Type::LIMIT_BUY,
                        Order::Type::LIMIT_SELL,
                        "O1",
                        "O2",
                        '5', // Decision '5' means this is a trade update from TRTH
                        "TRTH",
                        now,
                        now });
                }
                break;
            }
            case Order::Type::TRTH_BID: {
                if (newOrder.getDestination() == bidExchange) {
                    if (newOrder.getPrice() == bidPrice && newOrder.getSize() == bidSize) {
                        break;
                    }
                }
                bidExchange = newOrder.getDestination();
                bidPrice = newOrder.getPrice();
                bidSize = newOrder.getSize();
                stock->second.doLimitBuy(newOrder, "SHIFT");
                if (newOrder.getSize() != 0)
                    stock->second.updateGlobalBids(newOrder);
                break;
            }
            case Order::Type::TRTH_ASK: {
                if (newOrder.getDestination() == askExchange) {
                    if (newOrder.getPrice() == askPrice && newOrder.getSize() == askSize) {
                        break;
                    }
                }
                askExchange = newOrder.getDestination();
                askPrice = newOrder.getPrice();
                askSize = newOrder.getSize();
                stock->second.doLimitSell(newOrder, "SHIFT");
                if (newOrder.getSize() != 0)
                    stock->second.updateGlobalAsks(newOrder);
                break;
            }
            }

            for (const auto& report : stock->second.executionReports) {
                FIXAcceptor::getInstance()->sendExecutionReport2All(report);
            }
            stock->second.executionReports.clear();

            for (const auto& entry : stock->second.orderBookUpdates) {
                FIXAcceptor::getInstance()->sendOrderBookUpdate2All(entry);
            }
            stock->second.orderBookUpdates.clear();
        }
    }
}
