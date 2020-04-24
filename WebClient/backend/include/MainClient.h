#pragma once

#include <atomic>
#include <string>

#include <shift/coreclient/CoreClient.h>

class MainClient : public shift::CoreClient {
public:
    static std::atomic<bool> s_isTimeout;

    MainClient(const std::string& username);

    void sendAllPortfoliosToFront();
    void sendAllSubmittedOrders();
    void sendAllWaitingList();
    void sendLastPriceToFront();
    void sendOverviewInfoToFront();
    void sendOrderBookToFront();
    void sendStockListToFront();
    void sendCompanyNamesToFront();

    void sendOnce(const std::string& category);

    void receiveRequestFromPHP();
    void checkEverySecond();

protected:
    void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) override;

private:
    void debugDump(const std::string& message);

    bool m_openBuyingPowerReady = false;
    size_t m_setBuyingPowerNumber = 0;
};
