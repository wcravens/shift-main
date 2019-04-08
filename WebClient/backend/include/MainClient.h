#pragma once

#include <string>

#include <shift/coreclient/CoreClient.h>

class MainClient : public shift::CoreClient {
private:
    bool m_openBuyingPowerReady = false;
    size_t m_setBuyingPowerNumber = 0;

    void debugDump(const std::string& message);

public:
    MainClient(const std::string& username);
    void sendDBLoginToFront(const std::string& cryptoKey, const std::string& fileName);
    void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) override;
    void sendAllPortfoliosToFront();
    void sendAllSubmittedOrders();
    void sendAllWaitingList();
    void sendOrderBookToFront();
    void sendStockListToFront();
    void sendCompanyNamesToFront();
    void receiveRequestFromPHP();
    void sendOverviewInfoToFront();
    void sendLastPriceToFront();
    void checkEverySecond();
    void sendOnce(const std::string& category);
};
