#pragma once

#include "CoreClient_EXPORTS.h"

#include <string>

namespace shift {

class CORECLIENT_EXPORTS Report {

public:
    Report();

    Report(std::string clientID, std::string orderID, std::string symbol, double price, char orderType,
        char orderStatus, int size, std::string executionTime, std::string serverTime);

private:
    std::string m_clientID; //!< ClientID of the client who submit previous orders
    std::string m_orderID; //!< The orderID to be printed in the report.
    std::string m_symbol;
    double m_price;
    char m_orderType;
    char m_orderStatus; //!< order orderStatus: '1':confirmed; '2':partially filled; '3':canceled
    int m_size;
    std::string m_executionTime;
    std::string m_serverTime; //!< Order execution time corresponding to the server time at that moment.
};

} // shift
