#include "MainClient.h"

#include "MyZMQ.h"
#include "UserClient.h"

#include <shift/miscutils/terminal/Common.h>

MainClient::MainClient(const std::string& username)
    : CoreClient{ username }
{
}

void MainClient::debugDump(const std::string& message)
{
    cout << "***From MainClient***" << endl;
    cout << message << endl;
}

void MainClient::receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) //override
{
    std::ostringstream out;
    out << "{ \"category\": \"candleData_" << symbol << "\", \"data\":{ "
        << "\"symbol\": "
        << "\"" << symbol << "\","
        << "\"open\": "
        << "\"" << open << "\","
        << "\"high\": "
        << "\"" << high << "\","
        << "\"low\": "
        << "\"" << low << "\","
        << "\"close\": "
        << "\"" << close << "\","
        << "\"time\": "
        << "\"" << timestamp << "\""
        << "} }";
    MyZMQ::getInstance().send(out.str());
}

void MainClient::sendAllPortfoliosToFront()
{
    // TODO: may have problem
    for (auto& client : getAttachedClients()) {
        UserClient* wclient = dynamic_cast<UserClient*>(client);
        wclient->sendPortfolioToFront();
    }
}

void MainClient::sendAllSubmittedOrders()
{
    for (auto& client : getAttachedClients()) {
        UserClient* wclient = dynamic_cast<UserClient*>(client);
        wclient->sendSubmittedOrders();
    }
}

void MainClient::sendAllWaitingList()
{
    for (auto& client : getAttachedClients()) {
        UserClient* wclient = dynamic_cast<UserClient*>(client);
        wclient->receiveWaitingList();
    }
}

void MainClient::sendOrderBookToFront()
{
    for (const std::string& symbol : getStockList()) {
        for (const char& type : { 'a', 'A', 'b', 'B' }) {
            auto orderBook = getOrderBook(symbol, (shift::OrderBook::Type)type); // TODO: add maxLevel parameter = 5
            std::set<double> myset; // TODO: delete
            std::string res = "";
            for (const auto& entry : orderBook) {
                myset.insert(entry.getPrice()); // TODO: delete
                if (myset.size() > 5) // TODO: delete
                    break;
                std::ostringstream out;
                out << "{ "
                    << "\"bookType\": "
                    << "\"" << type << "\","
                    << "\"symbol\": "
                    << "\"" << entry.getSymbol() << "\","
                    << "\"price\": "
                    << "\"" << entry.getPrice() << "\","
                    << "\"size\": "
                    << "\"" << entry.getSize() << "\","
                    << "\"destination\": "
                    << "\"" << entry.getDestination() << "\","
                    << "\"time\": "
                    << "\"" << entry.getTime() << "\""
                    << "}";
                if (res == "") {
                    res += out.str();
                } else {
                    res += "," + out.str();
                }
            }
            if (res == "") {
                res = "{ \"bookType\": \"" + std::string(1, type) + "\",\"size\": \"0\"}";
            }
            double lastPrice = getLastPrice(symbol);
            double diff = lastPrice - getOpenPrice(symbol);
            std::ostringstream out;
            out << "{ "
                << "\"category\":"
                << "\"orderBook_" << symbol << "\","
                << "\"lastPrice\": "
                << "\"" << lastPrice << "\","
                << "\"rate\": "
                << "\"" << ((getOpenPrice(symbol) == 0) ? 0.0 : diff / getOpenPrice(symbol)) << "\","
                << "\"diff\": "
                << "\"" << diff << "\","
                << "\"data\":["
                << res
                << "]"
                << "}";
            MyZMQ::getInstance().send(out.str());
        }
    }
}

// only called by Default Client (Default(main) CLient works like instance of FIXInitiator)
void MainClient::receiveRequestFromPHP()
{
    // TODO: really bad, modify later
    while (1) {
        MyZMQ::getInstance().receiveReq();
    }
}

void MainClient::sendOverviewInfoToFront()
{
    for (const std::string& symbol : getStockList()) {
        shift::BestPrice price = getBestPrice(symbol);
        std::ostringstream out;
        out << "{\"category\": \"bestPrice\", \"data\":{ "
            << "\"symbol\": "
            << "\"" << symbol << "\","
            << "\"lastPrice\": "
            << "\"" << getLastPrice(symbol) << "\","
            << "\"best_bid_price\": "
            << "\"" << price.getBidPrice() << "\","
            << "\"best_bid_size\": "
            << "\"" << price.getBidSize() << "\","
            << "\"best_ask_price\": "
            << "\"" << price.getAskPrice() << "\","
            << "\"best_ask_size\": "
            << "\"" << price.getAskSize() << "\""
            << "} }";
        MyZMQ::getInstance().send(out.str());
    }
}

void MainClient::sendLastPriceToFront()
{
    for (auto symbol : getStockList()) {
        double lastPrice = getLastPrice(symbol);
        double diff = lastPrice - getOpenPrice(symbol);
        std::ostringstream out;
        out << "{\"category\": \"lastPriceView_" << symbol << "\", \"data\":{ "
            << "\"lastPrice\": "
            << "\"" << lastPrice << "\","
            << "\"rate\": "
            << "\"" << ((getOpenPrice(symbol) == 0) ? 0.0 : diff / getOpenPrice(symbol)) << "\","
            << "\"diff\": "
            << "\"" << diff << "\""
            << "} }";
        MyZMQ::getInstance().send(out.str());
    }
}

void MainClient::checkEverySecond()
{
    while (1) {
        sendAllPortfoliosToFront();
        // in 10^-6 seconds
        usleep(166666);
        sendOrderBookToFront();
        usleep(166666);
        sendLastPriceToFront();
        sendOverviewInfoToFront();
        usleep(166666);
    }
}

void MainClient::sendOnce(const std::string& category)
{
    if (category.find("portfolioSummary") != std::string::npos) {
        sendAllPortfoliosToFront();
    } else if (category.find("bestPriceView") != std::string::npos) {
        sendOverviewInfoToFront();
    } else if (category.find("orderBook") != std::string::npos) {
        sendOrderBookToFront();
    } else if (category.find("lastPriceView") != std::string::npos) {
        sendLastPriceToFront();
    } else if (category.find("leaderboard") != std::string::npos) {
        sendAllPortfoliosToFront();
    } else if (category.find("submittedOrders") != std::string::npos) {
        sendAllSubmittedOrders();
    } else if (category.find("waitingList") != std::string::npos) {
        sendAllWaitingList();
    }
}

// To send stock list to front end after all stock information has been received
void MainClient::sendStockListToFront()
{
    std::ostringstream out;
    std::string res = "";
    for (auto stock : getStockList()) {
        std::ostringstream tem;
        tem << "\"" << stock << "\"";
        if (res == "") {
            res += tem.str();
        } else {
            res += "," + tem.str();
        }
    }
    out << "{\"category\": \"stockList\", \"data\":["
        << res
        << "] }";
    MyZMQ::getInstance().send(out.str());
}

// To send company names to front end after all information has been received
void MainClient::sendCompanyNamesToFront()
{
    std::ostringstream out;
    std::string res = "";
    auto size = getStockList().size();

    while (getCompanyNames().size() != size) {
        sleep(1);
    }

    for (const auto& companyName : getCompanyNames()) {
        std::ostringstream oss;
        oss << "{\"" << companyName.first << "\": \"" << companyName.second << "\"}";
        if (res == "") {
            res += oss.str();
        } else {
            res += "," + oss.str();
        }
    }
    out << "{\"category\": \"companyNames\", \"data\":["
        << res
        << "] }";
    MyZMQ::getInstance().send(out.str());
}
