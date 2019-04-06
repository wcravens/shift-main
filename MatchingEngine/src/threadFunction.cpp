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
        char type;
        Quote newQuote;

        if (!stock->second.getNewQuote(newQuote)) {
            std::this_thread::sleep_for(50ms);
            continue;
        } else {
            type = newQuote.getOrderType();

            switch (type) {
            case '1': // limit buy
            {
                stock->second.doLocalLimitBuy(newQuote, "SHIFT");
                // if (newQuote.getsize() != 0) stock->second.checkGlobalAsk(newQuote, "limit");
                if (newQuote.getSize() != 0) {
                    stock->second.bidInsert(newQuote);
                    cout << "Insert Bid" << endl;
                }
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            case '2': // limit sell
            {
                stock->second.doLocalLimitSell(newQuote, "SHIFT");
                // if (newQuote.getsize() != 0) stock->second.checkGlobalBid(newQuote, "limit");
                if (newQuote.getSize() != 0) {
                    stock->second.askInsert(newQuote);
                    cout << "Insert Ask" << endl;
                }
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            case '3': // market buy
            {
                stock->second.doLocalMarketBuy(newQuote, "SHIFT");
                // if (newQuote.getsize() != 0) stock->second.checkGlobalAsk(newQuote, "market");
                if (newQuote.getSize() != 0)
                    stock->second.bidInsert(newQuote);
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            case '4': // market sell
            {
                stock->second.doLocalMarketSell(newQuote, "SHIFT");
                // if (newQuote.getsize() != 0) stock->second.checkGlobalBid(newQuote, "market");
                if (newQuote.getSize() != 0)
                    stock->second.askInsert(newQuote);
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            case '5': // cancel bid
            {
                stock->second.doCancelBid(newQuote, "SHIFT");
                // stock->second.showLocal();
                // stock->second.showGlobal();
                std::cout << "Cancel Bid Done" << endl;
                break;
            }
            case '6': // cancel ask
            {
                stock->second.doCancelAsk(newQuote, "SHIFT");
                // stock->second.showLocal();
                // stock->second.showglobal();
                std::cout << "Cancel Ask Done" << endl;
                break;
            }
            case '7': // trade from TR
            {
                // cout << "Global trade update: " << newQuote.getSymbol() << endl;
                stock->second.doLimitBuy(newQuote, "SHIFT");
                stock->second.doLimitSell(newQuote, "SHIFT");
                // stock->second.showLocal();
                // stock->second.showGlobal();
                if (newQuote.getSize() != 0) {
                    // Decision '5' means this is a trade update from TRTH
                    auto now = globalTimeSetting.simulationTimestamp();
                    ExecutionReport report{
                        newQuote.getSymbol(),
                        newQuote.getPrice(),
                        newQuote.getSize(),
                        "T1",
                        "T2",
                        '1',
                        '2',
                        "o1",
                        "o2",
                        '5',
                        "TRTH",
                        newQuote.getTime(),
                        now,
                        now
                    };
                    stock->second.executionReports.push_back(report);
                }
                break;
            }
            case '8': // ask update from TR
            {
                // cout << "Global ask update: " << newQuote.getSymbol() << endl;
                if (newQuote.getDestination() == askExchange) {
                    if (newQuote.getPrice() == askPrice && newQuote.getSize() == askSize) {
                        break;
                    }
                }
                askExchange = newQuote.getDestination();
                askPrice = newQuote.getPrice();
                askSize = newQuote.getSize();
                stock->second.doLimitSell(newQuote, "SHIFT");
                if (newQuote.getSize() != 0)
                    stock->second.updateGlobalAsk(newQuote);
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            case '9': // bid update from TR
            {
                if (newQuote.getDestination() == bidExchange) {
                    if (newQuote.getPrice() == bidPrice && newQuote.getSize() == bidSize) {
                        break;
                    }
                }
                bidExchange = newQuote.getDestination();
                bidPrice = newQuote.getPrice();
                bidSize = newQuote.getSize();
                stock->second.doLimitBuy(newQuote, "SHIFT");
                if (newQuote.getSize() != 0)
                    stock->second.updateGlobalBid(newQuote);
                // stock->second.showLocal();
                // stock->second.showGlobal();
                break;
            }
            }

            for (const auto& report : stock->second.executionReports) {
                // Send execution report to DatafeedEngine
                // Decision '5' means this is a trade update from TRTH -> no need to send it to DatafeedEngine
                if (report.decision != '5') {
                    FIXInitiator::getInstance()->sendExecutionReport(report);
                }
                // Send execution report to BrokerageCenter
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
