#include "configFunctions.h"

#include <fstream>
#include <sstream>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

bool fileConfigMode(std::string filePath, std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols)
{
    std::string line = "";
    std::string key = "";
    std::string value = "";

    std::ifstream inputFile(filePath);

    if (inputFile.is_open()) {
        cout << "Content of configuration file:" << endl;
        while (std::getline(inputFile, line)) {
            std::istringstream inputLine(line);
            if (std::getline(inputLine, key, '=')) {
                if (key[0] == '#')
                    continue;
                if (std::getline(inputLine, value)) {
                    if (key == "stock")
                        symbols.push_back(value);
                    else if (key == "date")
                        date = value;
                    else if (key == "starttime")
                        startTime = value;
                    else if (key == "endtime")
                        endTime = value;
                    else if (key == "speed")
                        experimentSpeed = stoi(value);
                    cout << value << endl;
                }
            }
        }
    } else {
        cout << "Cannot open " << filePath << '.' << endl;
        return false;
    }

    return true;
}

void inputConfigMode(std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols)
{
    cout << "Please input simulation date (format: yyyy-mm-dd, e.g 2018-12-17):" << endl;
    cin >> date;
    cout << endl;

    cout << "Please input start time (format: hh:mm:ss, from 09:30:00 to 15:59:00):" << endl;
    cin >> startTime;
    cout << endl;

    cout << "Please input end time (format:hh:mm:ss, from 15:59:00 to 16:00:00):" << endl;
    cin >> endTime;
    cout << endl;

    cout << "Please input experiment speed (1 is real time speed, 2 is double speed):" << endl;
    cin >> experimentSpeed;
    cout << endl;

    cout << "Please refer to http://www.reuters.com/finance/stocks/lookup for stock tickers." << endl;

    int numStocks = 1; // record the number of inputted stocks
    std::string symbol = "";

    while (1) {
        cout << endl
             << "Please input stock number " << numStocks << " (to stop input, please type '0'):" << endl;
        cin >> symbol;
        // exit when input is "0":
        if (symbol == "0")
            break;
        symbols.push_back(symbol);
        ++numStocks;
    }
}
