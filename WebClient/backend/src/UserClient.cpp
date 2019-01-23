#include "UserClient.h"

#include "MyZMQ.h"

#include <cmath>

#include <shift/miscutils/terminal/Common.h>

UserClient::UserClient(const std::string& username)
    : CoreClient{ username }
{
}

inline void UserClient::debugDump(const std::string& message)
{
    cout << "***From UserClient***" << endl;
    cout << message << endl;
}

inline double UserClient::decimalTruncate(double value, int precision)
{
    return std::round(value * std::pow(10, precision)) / std::pow(10, precision);
}

void UserClient::receiveWaitingList()
{
    auto waitingList = getWaitingList();
    std::string res = "";

    for (const auto& order : waitingList) {
        std::ostringstream out;
        out << "{ "
            << "\"orderId\": "
            << "\"" << order.getID() << "\","
            << "\"symbol\": "
            << "\"" << order.getSymbol() << "\","
            << "\"price\": "
            << "\"" << order.getPrice() << "\","
            << "\"shares\": "
            << "\"" << order.getSize() << "\","
            << "\"orderType\": "
            << "\"" << char(order.getType()) << "\""
            << "}";
        if (res == "") {
            res += out.str();
        } else {
            res += "," + out.str();
        }
    }

    std::ostringstream out;
    out << "{ \"category\": \"waitingList_" << getUsername() << "\", \"data\":[" << res << "]}";
    std::string s = out.str();
    debugDump(s);
    MyZMQ::getInstance().send(s);
}

void UserClient::sendSubmittedOrders()
{
    auto submittedOrders = getSubmittedOrders();
    std::string res = "";

    for (const auto& order : submittedOrders) {
        std::ostringstream out;

        // use this
        std::time_t timestamp = std::chrono::system_clock::to_time_t(order.getTimestamp());
        std::string timestampStr = std::ctime(&timestamp);
        timestampStr.pop_back();

        out << "{ "
            << "\"orderId\": "
            << "\"" << order.getID() << "\","
            << "\"symbol\": "
            << "\"" << order.getSymbol() << "\","
            << "\"price\": "
            << "\"" << order.getPrice() << "\","
            << "\"shares\": "
            << "\"" << order.getSize() << "\","
            << "\"orderType\": "
            << "\"" << char(order.getType()) << "\","
            << "\"timestamp\": "
            << "\"" << timestampStr << "\""
            << "}";
        if (res == "") {
            res += out.str();
        } else {
            res += "," + out.str();
        }
    }

    std::ostringstream out;
    out << "{ \"category\": \"submittedOrders_" << getUsername() << "\", \"data\":[" << res << "]}";
    std::string s = out.str();
    debugDump(s);
    MyZMQ::getInstance().send(s);
}

void UserClient::sendPortfolioToFront()
{
    // this function send its own portfolio to front
    std::string username = getUsername();

    auto portfolioSummary = getPortfolioSummary();
    double portfolioUnrealizedPL = 0.0;
    double totalPL = 0.0;
    std::string res = "";

    if (portfolioSummary.isOpenBPReady()) {
        auto portfolioItems = getPortfolioItems();
        for (auto it = portfolioItems.begin(); it != portfolioItems.end(); ++it) {
            std::string symbol = it->second.getSymbol();
            int currentShares = it->second.getShares();

            debugDump(username + " " + symbol + " " + std::to_string(currentShares));

            double tradedPrice = it->second.getPrice();
            bool buy = (currentShares < 0);
            double closePrice = (currentShares == 0) ? 0.0 : getClosePrice(symbol, buy, currentShares / 100);
            double unrealizedPL = (closePrice - tradedPrice) * currentShares;
            portfolioUnrealizedPL += unrealizedPL;
            auto pl = decimalTruncate(it->second.getRealizedPL() + unrealizedPL, 2);

            std::ostringstream out;
            out << "{ "
                << "\"symbol\": "
                << "\"" << symbol << "\","
                << "\"shares\": "
                << "\"" << currentShares << "\","
                << "\"price\": "
                << "\"" << tradedPrice << "\","
                << "\"closePrice\": "
                << "\"" << closePrice << "\","
                << "\"unrealizedPL\": "
                << "\"" << unrealizedPL << "\","
                << "\"pl\": "
                << "\"" << pl << "\""
                << "}";
            if (res == "") {
                res += out.str();
            } else {
                res += "," + out.str();
            }
        }
        std::ostringstream out;
        out << "{ \"category\": \"portfolio_" << username << "\", \"data\":[" << res << "]}";
        MyZMQ::getInstance().send(out.str());
    }

    totalPL = decimalTruncate(portfolioSummary.getTotalRealizedPL() + portfolioUnrealizedPL, 2);

    std::ostringstream out;
    out << std::fixed << "{\"category\": \"portfolioSummary_" << username << "\", \"data\":{ "
        << "\"totalBP\": "
        << "\"" << portfolioSummary.getTotalBP() << "\","
        << "\"totalShares\": "
        << "\"" << portfolioSummary.getTotalShares() << "\","
        << "\"portfolioUnrealizedPL\": "
        << "\"" << portfolioUnrealizedPL << "\","
        << "\"totalPL\": "
        << "\"" << totalPL << "\","
        << "\"earnings\": "
        << "\"" << totalPL / portfolioSummary.getOpenBP() << "\""
        << "} }";
    MyZMQ::getInstance().send(out.str());
}
