#include "configFunction.h"

#include "TimeSetting.h"

#include "FIXAcceptor.h"
#include "FIXInitiator.h"
#include "StockMarket.h"

#include <atomic>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

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
