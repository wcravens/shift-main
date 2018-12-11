#pragma once

#include "../service/thrift/gen-cpp/SHIFTService.h"

#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

/**
 * @brief SHIFT WebClient Thrift service Handler
 */

class SHIFTServiceHandler : public SHIFTServiceIf {
public:
    void submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, int32_t orderSize, double orderPrice, const std::string& orderID);
    void webClientSendUsername(const std::string& username);
    void webUserLogin(const std::string& username);
    // void startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
    // void DummyStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
};
