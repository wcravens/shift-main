#pragma once

#include <string>

#include <shift/coreclient/CoreClient.h>

class UserClient : public shift::CoreClient {
private:
    void debugDump(const std::string& message);
    double decimalTruncate(double value, int precision);

public:
    UserClient(const std::string& username);
    void receiveWaitingList() override;
    void sendSubmittedOrders();
    void sendPortfolioToFront();
};
