#include "MainClient.h"

#include "MyZMQ.h"
#include "UserClient.h"

#include <fstream>
#include <thread>

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

/* static */ std::atomic<bool> MainClient::s_isTimeout { false };

MainClient::MainClient(std::string username)
    : CoreClient { std::move(username) }
{
}

void MainClient::sendAllPortfoliosToFront()
{
    // TODO: may have a problem
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

void MainClient::sendLastPriceToFront()
{
    for (const std::string& symbol : getStockList()) {
        double lastPrice = getLastPrice(symbol);
        double diff = lastPrice - getOpenPrice(symbol);
        std::time_t simulationTime = std::chrono::system_clock::to_time_t(getLastTradeTime());
        std::ostringstream out;
        out << "{\"category\": \"lastPriceView_" << symbol << "\", \"data\":{ "
            << "\"lastPrice\": "
            << "\"" << lastPrice << "\","
            << "\"diff\": "
            << "\"" << diff << "\","
            << "\"rate\": "
            << "\"" << ((getOpenPrice(symbol) == 0) ? 0.0 : diff / getOpenPrice(symbol)) << "\","
            << "\"simulationTime\": "
            << "\"" << std::put_time(std::localtime(&simulationTime), "%F %T") << "\""
            << "} }";
        MyZMQ::getInstance().send(out.str());
    }
}

void MainClient::sendOverviewInfoToFront()
{
    for (const std::string& symbol : getStockList()) {
        double lastPrice = getLastPrice(symbol);
        shift::BestPrice bestPrice = getBestPrice(symbol);
        std::ostringstream out;
        out << "{\"category\": \"bestPrice\", \"data\":{ "
            << "\"symbol\": "
            << "\"" << symbol << "\","
            << "\"lastPrice\": "
            << "\"" << lastPrice << "\","
            << "\"best_bid_price\": "
            << "\"" << bestPrice.getBidPrice() << "\","
            << "\"best_bid_size\": "
            << "\"" << bestPrice.getBidSize() << "\","
            << "\"best_ask_price\": "
            << "\"" << bestPrice.getAskPrice() << "\","
            << "\"best_ask_size\": "
            << "\"" << bestPrice.getAskSize() << "\""
            << "} }";
        MyZMQ::getInstance().send(out.str());
    }
}

void MainClient::sendOrderBookToFront()
{
    for (const std::string& symbol : getStockList()) {
        for (const char& type : { 'a', 'A', 'b', 'B' }) {
            auto orderBook = getOrderBook(symbol, static_cast<shift::OrderBook::Type>(type), 5);
            std::string res;
            for (const auto& entry : orderBook) {
                std::time_t timeTmp = std::chrono::system_clock::to_time_t(entry.getTime());
                std::ostringstream out;
                out << "{ "
                    << "\"symbol\": "
                    << "\"" << symbol << "\","
                    << "\"bookType\": "
                    << "\"" << type << "\","
                    << "\"price\": "
                    << "\"" << entry.getPrice() << "\","
                    << "\"size\": "
                    << "\"" << entry.getSize() << "\","
                    << "\"destination\": "
                    << "\"" << entry.getDestination() << "\","
                    << "\"time\": "
                    << "\"" << timeTmp << "\""
                    << "}";
                if (res.empty()) {
                    res += out.str();
                } else {
                    res += "," + out.str();
                }
            }
            if (res.empty()) {
                res = "{ \"bookType\": \"" + std::string(1, type) + "\",\"size\": \"0\"}";
            }
            std::ostringstream out;
            out << "{ "
                << "\"category\":"
                << "\"orderBook_" << symbol << "\","
                << "\"data\":["
                << res
                << "]"
                << "}";
            MyZMQ::getInstance().send(out.str());
        }
    }
}

/**
 * @brief To send stock list to front end after all stock information has been received.
 */
void MainClient::sendStockListToFront()
{
    std::ostringstream out;
    std::string res;
    for (const std::string& symbol : getStockList()) {
        std::ostringstream temp;
        temp << "\"" << symbol << "\"";
        if (res.empty()) {
            res += temp.str();
        } else {
            res += "," + temp.str();
        }
    }
    out << "{\"category\": \"stockList\", \"data\":["
        << res
        << "] }";
    MyZMQ::getInstance().send(out.str());
}

/**
 * @brief To send company names to front end after all information has been received.
 */
void MainClient::sendCompanyNamesToFront()
{
    std::ostringstream out;
    std::string res;
    auto size = getStockList().size();
    while (getCompanyNames().size() != size) {
        std::this_thread::sleep_for(1s);
    }
    for (const auto& [ticker, companyName] : getCompanyNames()) {
        std::ostringstream oss;
        oss << "{\"" << ticker << "\": \"" << companyName << "\"}";
        if (res.empty()) {
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

void MainClient::receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) // override
{
    std::ostringstream out;
    out << "{ \"category\": \"candlestickData_" << symbol << "\", \"data\":{ "
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

void MainClient::receiveRequestFromPHP()
{
    while (!s_isTimeout) {
        MyZMQ::getInstance().receiveReq();
    }
}

void MainClient::checkEverySecond()
{
    while (!s_isTimeout) {
        sendLastPriceToFront();
        sendOverviewInfoToFront();
        std::this_thread::sleep_for(0.15s);
        sendOrderBookToFront();
        std::this_thread::sleep_for(0.15s);
        sendAllPortfoliosToFront();
        std::this_thread::sleep_for(0.15s);
    }
}

inline void MainClient::debugDump(const std::string& message) const
{
    cout << "***From MainClient***" << endl;
    cout << message << endl;
}
