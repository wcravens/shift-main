#pragma once

#include "../service/thrift/gen-cpp/SHIFTService.h"

#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

/**
 * @brief SHIFT WebClient Thrift service Handler.
 */
class SHIFTServiceHandler : public SHIFTServiceIf {
public:
    void submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, int32_t orderSize, double orderPrice, const std::string& orderID);
    void getAllTraders(std::string& _return);
    void getThisLeaderboard(std::string& _return, const std::string& startDate, const std::string& endDate);
    void getThisLeaderboardByDay(std::string& _return, int32_t contestDay);
    void webClientSendUsername(const std::string& username);
    void webUserLoginV2(std::string& _return, const std::string& username, const std::string& password);
    void registerUser(std::string& _return, const std::string& username, const std::string& firstname, const std::string& lastname, const std::string& email, const std::string& password);
    void is_login(std::string& _return, const std::string& sessionid);
    void change_password(std::string& _return, const std::string& cur_password, const std::string& new_password, const std::string& username);
    void get_user_by_username(std::string& _return, const std::string& username);
    void webUserLogin(const std::string& username);

};
