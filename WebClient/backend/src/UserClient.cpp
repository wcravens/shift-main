#include "UserClient.h"

#include "MyZMQ.h"

#include <cmath>

#include <shift/miscutils/terminal/Common.h>

UserClient::UserClient(const std::string& username)
    : CoreClient { username }
{
}

/**
 * @brief To send the user's portfolio to front.
 */
void UserClient::sendPortfolioToFront()
{
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
            auto pl = roundNearest(it->second.getRealizedPL() + unrealizedPL, 0.01);

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
        MyZMQ::getInstance()->send(out.str());
    }

    totalPL = roundNearest(portfolioSummary.getTotalRealizedPL() + portfolioUnrealizedPL, 0.01);

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
    MyZMQ::getInstance()->send(out.str());
}

void UserClient::sendSubmittedOrders()
{
    auto submittedOrders = getSubmittedOrders();
    std::string res = "";

    double price = 0.0;
    std::time_t timestamp = 0;
    std::string timestampStr = "";

    for (const auto& order : submittedOrders) {
        std::ostringstream out;

        if (order.getStatus() == shift::Order::Status::FILLED) {
            price = order.getExecutedPrice();
        } else {
            price = order.getPrice();
        }

        timestamp = std::chrono::system_clock::to_time_t(order.getTimestamp());
        timestampStr = std::ctime(&timestamp);
        timestampStr.pop_back();

        out << "{ "
            << "\"orderType\": "
            << "\"" << order.getTypeString() << "\","
            << "\"symbol\": "
            << "\"" << order.getSymbol() << "\","
            << "\"size\": "
            << "\"" << order.getSize() << "\","
            << "\"executedSize\": "
            << "\"" << order.getExecutedSize() << "\","
            << "\"price\": "
            << "\"" << price << "\","
            << "\"orderId\": "
            << "\"" << order.getID() << "\","
            << "\"status\": "
            << "\"" << order.getStatusString() << "\","
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
    MyZMQ::getInstance()->send(s);
}

void UserClient::receiveWaitingList()
{
    auto waitingList = getWaitingList();
    std::string res = "";

    for (const auto& order : waitingList) {
        std::ostringstream out;
        out << "{ "
            << "\"orderType\": "
            << "\"" << order.getTypeString() << "\","
            << "\"symbol\": "
            << "\"" << order.getSymbol() << "\","
            << "\"size\": "
            << "\"" << (order.getSize() - order.getExecutedSize()) << "\","
            << "\"price\": "
            << "\"" << order.getPrice() << "\","
            << "\"orderId\": "
            << "\"" << order.getID() << "\","
            << "\"status\":"
            << "\"" << order.getStatusString() << "\""
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
    MyZMQ::getInstance()->send(s);
}

inline double UserClient::roundNearest(double value, double nearest)
{
    return std::round(value / nearest) * nearest;
}

inline void UserClient::debugDump(const std::string& message)
{
    cout << "***From UserClient***" << endl;
    cout << message << endl;
}
