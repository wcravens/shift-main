/* 
** DBConnector maintains the connection between BC and database
*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <postgresql/libpq-fe.h>

struct UserInfo {
    std::string id;
    std::string fistName;
    std::string lastName;
    std::string accountName;
    std::string password;
    double buyingPower;
    std::string email;
    char gender;
};

class DBConnector {
public:
    ~DBConnector();

    static DBConnector* instance();
    bool init(const std::string& cryptoKey, const std::string& fileName);
    PGconn* getConn() { return m_pConn; } // establish connection to database
    bool connectDB();
    void disconnectDB();
    bool createClients(const std::string& symbol);

private:
    DBConnector(); /* singleton pattern */
    std::unordered_map<std::string, std::string> m_loginInfo;

protected:
    PGconn* m_pConn;
};

std::vector<std::string> readCol(const std::string& sql);
