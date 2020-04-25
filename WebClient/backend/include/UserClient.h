#pragma once

#include <string>

#include <shift/coreclient/CoreClient.h>

class UserClient : public shift::CoreClient {
public:
    UserClient(const std::string& username);

    void sendPortfolioToFront();
    void sendSubmittedOrders();

    void receiveWaitingList() override;

private:
    void debugDump(const std::string& message);
};
