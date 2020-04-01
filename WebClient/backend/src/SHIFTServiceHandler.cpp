#include "SHIFTServiceHandler.h"

#include "UserClient.h"

#include <thread>
#include <ctime>

#include <shift/coreclient/CoreClient.h>
#include <shift/coreclient/FIXInitiator.h>
#include <shift/coreclient/Order.h>

#include "DBConnector.h"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <shift/miscutils/crossguid/Guid.h>

using json = nlohmann::json;

/**
 * @brief: parses out a pRes object result and jsonifies it.
 * @param: pRes, pointer to a pGSQL 
 * @TODO: Probably move this somewhere more multi-purpose? Other modules can utilize this.
 */
json parsePresult(PGresult* pRes){
    json j;
    int rows = PQntuples(pRes);
    int fields = PQnfields(pRes);

    std::vector<std::vector<std::string>> vs;
    for(int row = 0; row < rows; ++row){
        std::vector<std::string> cv;
        for(int field = 0; field < fields; ++field){
            cv.push_back(PQgetvalue(pRes,row,field));
        }
        vs.push_back(cv);
    }

    //This library is pretty cool, huh
    j["rowCount"] = rows;
    j["fieldCount"] = fields;
    j["data"] = vs;

    return j;
}

/**
 * 
 * 0:id, 1:username, 2:firstname, 3:lastname, 4:email, 5:role, 6:sessionid, 7:super
 */ 
json parseProfile(PGresult* pRes){
    json j;

    j["id"] = PQgetvalue(pRes,0,0);
    j["username"] = PQgetvalue(pRes,0,1);
    j["firstname"] = PQgetvalue(pRes,0,2);
    j["lastname"] = PQgetvalue(pRes,0,3);
    j["email"] = PQgetvalue(pRes,0,4);
    j["role"] = PQgetvalue(pRes,0,5);
    j["sessionid"] = PQgetvalue(pRes,0,6);
    j["super"] = PQgetvalue(pRes,0,7);

    return j;
}

/**
 * @brief: SHA1 Encrypts a string.
 * @param: str: std::string to encrypt
 * @return: hex representation of the string that has been encrypted
 * @TODO: Probably move this somewhere more multi-purpose? Other modules can utilize this.
 */

std::string encryptStr(const std::string& str){

    std::cout << "ENCRYPTING " << str << std::endl;
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA_CTX shactx;
    SHA1_Init(&shactx);

    SHA1_Update(&shactx, str.c_str(), str.size());
    SHA1_Final(digest, &shactx);

    //C++ stringstreams are the way to go
    std::stringstream strm;
    strm << std::hex << std::setfill('0');
    for(int i =0; i < SHA_DIGEST_LENGTH; i++){
        strm << std::setw(2) << static_cast<int>(digest[i]);
    }
    
    return strm.str();
}

/**
 * @brief Method for submitting orders to BC.
 */
void SHIFTServiceHandler::submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, int32_t orderSize, double orderPrice = 0.0, const std::string& orderID = "")
{
    shift::Order::Type type = shift::Order::Type(orderType[0]);
    shift::Order order(type, orderSymbol, orderSize, orderPrice); // new order IDs are generated by the constructor of Order
    if (type == shift::Order::Type::CANCEL_BID || type == shift::Order::Type::CANCEL_ASK) {
        order.setID(orderID);
    }
    shift::CoreClient* ccptr = shift::FIXInitiator::getInstance().getClient(username);
    if (ccptr)
        ccptr->submitOrder(order);
}

/**
 * @brief Method for sending a list of all active traders, and their properties
 * @param _return Thrift default param that all thrift service functions have in their virtual void funcs
 */
void SHIFTServiceHandler::getAllTraders(std::string& _return){ //NOTE: Thrift modules deposit the result in this _return value param, for more complicated data structs
    //Probably make this last param some kind of default value..?

    PGresult* pRes;

    if(DBConnector::getInstance()->doQuery("SELECT id, username, email, role, super from traders", "FAILED QUERY\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
    }
    else{
        std::cout << "COULD NOT GET??" << std::endl;
    }

    std::cout << "Hello from shift!" << std::endl;
    if(DBConnector::b_hasConnected){
        std::cout << "DB has been initialized and is connected!" << std::endl;
    }
    else {
        std::cout << "Oh no...." << std::endl;
    }

    json j;
    j = parsePresult(pRes);
    auto s = j.dump(4);

    _return = s;

    std::cout << "returned... " << s << std::endl;
    //practice proper hygiene ^.^
    PQclear(pRes);
}

/**
 * @brief Method for sending a list of all active traders, and their properties
 * @param _return Thrift default param that all thrift service functions have in their virtual void funcs
 */
void SHIFTServiceHandler::getAllTraders(std::string& _return){ //NOTE: Thrift modules deposit the result in this _return value param, for more complicated data structs
    //Probably make this last param some kind of default value..?

    PGresult* pRes;

    if(DBConnector::getInstance()->doQuery("SELECT id, username, email, role, super from traders", "FAILED QUERY\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
    }
    else{
        std::cout << "COULD NOT GET??" << std::endl;
    }

    std::cout << "Hello from shift!" << std::endl;
    if(DBConnector::b_hasConnected){
        std::cout << "DB has been initialized and is connected!" << std::endl;
    }
    else {
        std::cout << "Oh no...." << std::endl;
    }

    json j;
    j = parsePresult(pRes);
    auto s = j.dump(4);

    _return = s;

    std::cout << "returned... " << s << std::endl;
    //practice proper hygiene ^.^
    PQclear(pRes);
}

/**
 * @brief Method for sending a list of the specified day's rankings
 * @param _return Thrift default param that all thrift service functions have in their virtual void funcs
 * @param dateRange Day of interest for rankings, in the format YYYY-MM-DD
 */
void SHIFTServiceHandler::getThisLeaderboard(std::string& _return, const std::string& startDate, const std::string& endDate){

    PGresult* pRes;

    struct tm tm;

    std::string validatedStart = "";
    std::string query;
    if(strptime(startDate.c_str(), "%Y-%m-%d",&tm) && strptime(endDate.c_str(), "%Y-%m-%d", &tm)){
        std::cout << "valid dates" << std::endl;
        query = std::string("SELECT rank, username, eod_buying_power, eod_traded_shares, eod_pl, pl_2, end_date, contest_day from leaderboard join traders on leaderboard.trader_id = traders.id where start_date > '") + startDate + std::string("' and end_date < '") + endDate + std::string("' ORDER BY rank asc;");
    }
    else{
        std::cout << "invalid dates!" << std::endl;
        query = "SELECT rank, username, eod_buying_power, eod_traded_shares, eod_pl, pl_2, end_date, contest_day from leaderboard join traders on leaderboard.trader_id = traders.id;";
    }

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE LEADERBOARD\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
    }
    else{
        std::cout << "COULD NOT GET??" << std::endl;
    }

    json j;
    j = parsePresult(pRes);
    auto s = j.dump(4);

    _return = s;

    std::cout << "returned... " << s << std::endl;
    //practice proper hygiene ^.^
    PQclear(pRes);

}

/**
 * @brief Method for sending a list of the specified day's rankings
 * @param _return Thrift default param that all thrift service functions have in their virtual void funcs
 * @param dateRange Day of interest for rankings, in the format YYYY-MM-DD
 */
void SHIFTServiceHandler::getThisLeaderboardByDay(std::string& _return, int32_t contestDay){

    PGresult* pRes;

    auto query = std::string("SELECT rank, username, eod_buying_power, eod_traded_shares, eod_pl, pl_2, end_date from leaderboard join traders on leaderboard.trader_id = traders.id where contest_day = ") + std::to_string(contestDay) + std::string(" ORDER BY rank asc;");

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE LEADERBOARD\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
    }
    else{
        std::cout << "COULD NOT GET??" << std::endl;
    }

    json j;
    j = parsePresult(pRes);
    auto s = j.dump(4);

    _return = s;

    std::cout << "returned... " << s << std::endl;
    //practice proper hygiene ^.^
    PQclear(pRes);

}

void SHIFTServiceHandler::registerUser(std::string& _return, const std::string& username, const std::string& firstname, const std::string& lastname, const std::string& email, const std::string& password)
{
    std::cout << "REGISTERUSER" << std::endl;
    PGresult* pRes;
    std::string shaPw = encryptStr(password);
    bool b_user_register_success = false;

    if(DBConnector::getInstance()->doQuery("SELECT * FROM traders where username = '" + username + "';", "COULD NOT RETRIEVE USERNAME; CREATING..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query failed: " << username << std::endl;
        json j;
        j["success"] = b_user_register_success;
        auto s = j.dump(4);

        std::cout << "returned... " << s << std::endl;
        _return = s;

    }
    if(PQntuples(pRes) == 0){
        const auto query = "INSERT INTO traders (username, password, firstname, lastname, email, role, super) VALUES ('"
            + username + "','"
            + shaPw + "','"
            + firstname + "','" + lastname + "','" + email 
            + "','student"// info
            + "',TRUE);";

        std::cout << query << std::endl;
        if(DBConnector::getInstance()->doQuery(query, "COULD NOT UPDATE SESSION ID\n")){
            std::cout << "new user: " << username << std::endl;
            b_user_register_success = true;
        }        

        json j;
        j["success"] = b_user_register_success;
        auto s = j.dump(4);

        std::cout << "returned... " << s << std::endl;
        _return = s;

    }

}

/**
 * @brief Method for sending a list of the specified day's rankings
 * @param _return Thrift default param that all thrift service functions have in their virtual void funcs
 * @param dateRange Day of interest for rankings, in the format YYYY-MM-DD
 */
void SHIFTServiceHandler::getThisLeaderboardByDay(std::string& _return, int32_t contestDay){

    PGresult* pRes;

    auto query = std::string("SELECT rank, username, eod_buying_power, eod_traded_shares, eod_pl, pl_2, end_date from leaderboard join traders on leaderboard.trader_id = traders.id where contest_day = ") + std::to_string(contestDay) + std::string(" ORDER BY rank asc;");

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE LEADERBOARD\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
    }
    else{
        std::cout << "COULD NOT GET??" << std::endl;
    }

    json j;
    j = parsePresult(pRes);
    auto s = j.dump(4);

    _return = s;

    std::cout << "returned... " << s << std::endl;
    //practice proper hygiene ^.^
    PQclear(pRes);

}

void SHIFTServiceHandler::registerUser(std::string& _return, const std::string& username, const std::string& firstname, const std::string& lastname, const std::string& email, const std::string& password)
{
    std::cout << "REGISTERUSER" << std::endl;
    PGresult* pRes;
    std::string shaPw = encryptStr(password);
    bool b_user_register_success = false;

    if(DBConnector::getInstance()->doQuery("SELECT * FROM traders where username = '" + username + "';", "COULD NOT RETRIEVE USERNAME; CREATING..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query failed: " << username << std::endl;
        json j;
        j["success"] = b_user_register_success;
        auto s = j.dump(4);

        std::cout << "returned... " << s << std::endl;
        _return = s;

    }
    if(PQntuples(pRes) == 0){
        const auto query = "INSERT INTO traders (username, password, firstname, lastname, email, role, super) VALUES ('"
            + username + "','"
            + shaPw + "','"
            + firstname + "','" + lastname + "','" + email 
            + "','student"// info
            + "',TRUE);";

        std::cout << query << std::endl;
        if(DBConnector::getInstance()->doQuery(query, "COULD NOT UPDATE SESSION ID\n")){
            std::cout << "new user: " << username << std::endl;
            b_user_register_success = true;
        }        

        json j;
        j["success"] = b_user_register_success;
        auto s = j.dump(4);

        std::cout << "returned... " << s << std::endl;
        _return = s;

    }

}

/**
 * @brief Method for sending current username to frontend.
 * @param username The current user who is operating on WebClient.
 */
void SHIFTServiceHandler::webUserLoginV2(std::string& _return, const std::string& username, const std::string& password)
{
    std::cout << "LOGGINGIN" << std::endl;
    PGresult* pRes;
    bool b_found_user = false;
    std::string sessionGuid = "";

    std::string shaPw = encryptStr(password);
    std::string query;
    query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where username = '" + username + "' and password = '" + shaPw + "' LIMIT 1;";
    //std::cout << query << std::endl;
    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE USER\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "RESULTS OBTAINED" << std::endl;
        b_found_user = true;

        sessionGuid = shift::crossguid::newGuid().str();
        query = "UPDATE traders SET sessionid = '" + sessionGuid + "' WHERE username = '" + username + "';";
        std::cout << query << std::endl;
        if(DBConnector::getInstance()->doQuery(query, "COULD NOT UPDATE SESSION ID\n")){
            std::cout << "new guid: " << sessionGuid << std::endl;
        }        
    }
    else{
        std::cout << "COULD NOT FIND USER" << std::endl;
    }

    if (username == "" && password == "")
        return;

    json j;
    j = parseProfile(pRes);
    j["success"] = b_found_user;
    j["sessionid"] = sessionGuid;

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

    //SHiFT Fix Login.
    try {
        shift::FIXInitiator::getInstance().getClient(username);
    } catch (...) {
        shift::CoreClient* ccptr = new UserClient(username);
        shift::FIXInitiator::getInstance().attachClient(ccptr);
    }
}

void SHIFTServiceHandler::is_login(std::string& _return, const std::string& sessionid){
    std::cout << "ISLOGIN" << std::endl;
    PGresult* pRes;
    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where sessionid = '" + sessionid + "';";
    json j;

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE USERNAME; CREATING..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query succeeded: " << sessionid << std::endl;
        j["success"] = false;

    }

    if(PQntuples(pRes) != 1){
        j["success"] = false;
    }
    else{
        j = parseProfile(pRes);
        j["success"] = true;
    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::change_password(std::string& _return, const std::string& cur_password, const std::string& new_password, const std::string& username){
    std::cout << "CHANGEPW" << std::endl;
    std::string shaCPw = encryptStr(cur_password);
    std::string newPw = encryptStr(new_password);

    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where password = '" + shaCPw + "' and username = '" + username + "';";
    json j;

    PGresult* eRes;
    if(DBConnector::getInstance()->doQuery(query, "COULD find user w/ password..\n", PGRES_TUPLES_OK, &eRes)){
        std::cout << "query succeeded: " << username << std::endl;
    }
    else{

        std::cout << "incorrect user/password" << std::endl;
    }


    if(PQntuples(eRes) != 1){
        std::cout << "query failed "  << shaCPw << " " << newPw << std::endl;
        j["success"] = false;
    }
    else {

        if(shaCPw.compare(newPw) != 0){
            const auto updateQuery = "UPDATE traders SET password = '"+newPw+"'"+  " WHERE username = '" + username + "';";
            if(DBConnector::getInstance()->doQuery(updateQuery, "COULD NOT UPDATE SESSION ID\n")){
                std::cout << "new pw saved: " << username << std::endl;
                j["success"] = true;
            }        

        }
        else{
            std::cout << "username attempted to change to same pw " << shaCPw << " " << newPw << std::endl;
            std::cout << "username attempted to change to same pw " << cur_password << " " << new_password << std::endl;

            j["success"] = false;
        }

    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::get_user_by_username(std::string& _return, const std::string& username){
    std::cout << "GETUSERBYNAME" << std::endl;
    PGresult* pRes;
    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where username = '" + username + "';";
    json j;

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE USER..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query succeeded: " << username << std::endl;
    }

    if(PQntuples(pRes) != 1){
        std::cout << "did not find user " << username << std::endl;
        j["success"] = false;
    }
    else{
        std::cout << "found user " << username << std::endl;
        j = parseProfile(pRes);
        j["success"] = true;
    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::is_login(std::string& _return, const std::string& sessionid){
    std::cout << "ISLOGIN" << std::endl;
    PGresult* pRes;
    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where sessionid = '" + sessionid + "';";
    json j;

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE USERNAME; CREATING..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query succeeded: " << sessionid << std::endl;
        j["success"] = false;

    }

    if(PQntuples(pRes) != 1){
        j["success"] = false;
    }
    else{
        j = parseProfile(pRes);
        j["success"] = true;
    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::change_password(std::string& _return, const std::string& cur_password, const std::string& new_password, const std::string& username){
    std::cout << "CHANGEPW" << std::endl;
    std::string shaCPw = encryptStr(cur_password);
    std::string newPw = encryptStr(new_password);

    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where password = '" + shaCPw + "' and username = '" + username + "';";
    json j;

    PGresult* eRes;
    if(DBConnector::getInstance()->doQuery(query, "COULD find user w/ password..\n", PGRES_TUPLES_OK, &eRes)){
        std::cout << "query succeeded: " << username << std::endl;
    }
    else{

        std::cout << "incorrect user/password" << std::endl;
    }


    if(PQntuples(eRes) != 1){
        std::cout << "query failed "  << shaCPw << " " << newPw << std::endl;
        j["success"] = false;
    }
    else {

        if(shaCPw.compare(newPw) != 0){
            const auto updateQuery = "UPDATE traders SET password = '"+newPw+"'"+  " WHERE username = '" + username + "';";
            if(DBConnector::getInstance()->doQuery(updateQuery, "COULD NOT UPDATE SESSION ID\n")){
                std::cout << "new pw saved: " << username << std::endl;
                j["success"] = true;
            }        

        }
        else{
            std::cout << "username attempted to change to same pw " << shaCPw << " " << newPw << std::endl;
            std::cout << "username attempted to change to same pw " << cur_password << " " << new_password << std::endl;

            j["success"] = false;
        }

    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::get_user_by_username(std::string& _return, const std::string& username){
    std::cout << "GETUSERBYNAME" << std::endl;
    PGresult* pRes;
    const auto query = "SELECT id, username, firstname, lastname, email, role, sessionid, super from traders where username = '" + username + "';";
    json j;

    if(DBConnector::getInstance()->doQuery(query, "COULD NOT RETRIEVE USER..\n", PGRES_TUPLES_OK, &pRes)){
        std::cout << "query succeeded: " << username << std::endl;
    }

    if(PQntuples(pRes) != 1){
        std::cout << "did not find user " << username << std::endl;
        j["success"] = false;
    }
    else{
        std::cout << "found user " << username << std::endl;
        j = parseProfile(pRes);
        j["success"] = true;
    }

    auto s = j.dump(4);

    std::cout << "returned... " << s << std::endl;
    _return = s;

}

void SHIFTServiceHandler::webClientSendUsername(const std::string& username)
{
    return;
}

/**
 * @brief Method for sending current username to frontend.
 * @param username The current user who is operating on WebClient.
 */
void SHIFTServiceHandler::webUserLogin(const std::string& username)
{
    if (username == "")
        return;

    try {
        shift::FIXInitiator::getInstance().getClient(username);
    } catch (...) {
        shift::CoreClient* ccptr = new UserClient(username);
        shift::FIXInitiator::getInstance().attachClient(ccptr);
    }
}

