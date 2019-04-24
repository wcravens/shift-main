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
    auto stockIt = (StockList::getInstance()).find(symbol);

    Order nextOrder;
    Order prevGlobalOrder;

    while (!timeout) { // process orders

        if (!stockIt->second.getNextOrder(nextOrder)) {
            std::this_thread::sleep_for(10ms);
            continue;

        } else {
            switch (nextOrder.getType()) {

            case Order::Type::LIMIT_BUY: {
                stockIt->second.doLocalLimitBuy(nextOrder);
                if (nextOrder.getSize() != 0) {
                    stockIt->second.insertLocalBid(nextOrder);
                    cout << "Insert Bid" << endl;
                }
                break;
            }

            case Order::Type::LIMIT_SELL: {
                stockIt->second.doLocalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    stockIt->second.insertLocalAsk(nextOrder);
                    cout << "Insert Ask" << endl;
                }
                break;
            }

            case Order::Type::MARKET_BUY: {
                stockIt->second.doLocalMarketBuy(nextOrder);
                if (nextOrder.getSize() != 0) {
                    stockIt->second.insertLocalBid(nextOrder);
                    cout << "Insert Bid" << endl;
                }
                break;
            }

            case Order::Type::MARKET_SELL: {
                stockIt->second.doLocalMarketSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    stockIt->second.insertLocalAsk(nextOrder);
                    cout << "Insert Ask" << endl;
                }
                break;
            }

            case Order::Type::CANCEL_BID: {
                stockIt->second.doLocalCancelBid(nextOrder);
                std::cout << "Cancel Bid Done" << endl;
                break;
            }

            case Order::Type::CANCEL_ASK: {
                stockIt->second.doLocalCancelAsk(nextOrder);
                std::cout << "Cancel Ask Done" << endl;
                break;
            }

            case Order::Type::TRTH_TRADE: {
                stockIt->second.doGlobalLimitBuy(nextOrder);
                stockIt->second.doGlobalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0) {
                    auto now = (TimeSetting::getInstance()).simulationTimestamp();
                    stockIt->second.executionReports.push_back({ nextOrder.getSymbol(),
                        nextOrder.getPrice(),
                        nextOrder.getSize(),
                        "T1",
                        "T2",
                        Order::Type::LIMIT_BUY,
                        Order::Type::LIMIT_SELL,
                        "O1",
                        "O2",
                        '5', // decision '5' means this is a trade update from TRTH
                        "TRTH",
                        now,
                        now });
                }
                break;
            }

            case Order::Type::TRTH_BID: {
                // If same price, size, and destination, skip
                if (nextOrder == prevGlobalOrder) {
                    break;
                }
                prevGlobalOrder = nextOrder;
                stockIt->second.doGlobalLimitBuy(nextOrder);
                if (nextOrder.getSize() != 0)
                    stockIt->second.updateGlobalBids(nextOrder);
                break;
            }

            case Order::Type::TRTH_ASK: {
                // If same price, size, and destination, skip
                if (nextOrder == prevGlobalOrder) {
                    break;
                }
                prevGlobalOrder = nextOrder;
                stockIt->second.doGlobalLimitSell(nextOrder);
                if (nextOrder.getSize() != 0)
                    stockIt->second.updateGlobalAsks(nextOrder);
                break;
            }
            }

            for (const auto& report : stockIt->second.executionReports) {
                FIXAcceptor::getInstance()->sendExecutionReport2All(report);
            }
            stockIt->second.executionReports.clear();

            for (const auto& entry : stockIt->second.orderBookUpdates) {
                FIXAcceptor::getInstance()->sendOrderBookUpdate2All(entry);
            }
            stockIt->second.orderBookUpdates.clear();
        }
    }
}
