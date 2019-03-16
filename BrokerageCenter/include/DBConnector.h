/*
** DBConnector maintains the connection between BC and database
*/

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <postgresql/libpq-fe.h>

class DBConnector {
public:
    static bool s_isPortfolioDBReadOnly; // E.g. useful for research purpose when true

    ~DBConnector();

    std::unique_lock<std::mutex> lockPSQL() const;

    static DBConnector* getInstance();
    bool init(const std::string& cryptoKey, const std::string& fileName);

    PGconn* getConn() { return m_pConn; } // establish connection to database
    bool connectDB();
    void disconnectDB();

    bool doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch = PGRES_COMMAND_OK, PGresult** ppRes = nullptr);

protected:
    PGconn* m_pConn;
    mutable std::mutex m_mtxPSQL;

private:
    DBConnector(); /* singleton pattern */
    std::unordered_map<std::string, std::string> m_loginInfo;
};
