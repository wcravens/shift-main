#pragma once

#include "TradingRecord.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <libpq-fe.h>

class DBConnector {
public:
    static bool s_isPortfolioDBReadOnly; // e.g. useful for research purpose when true
    static const std::string s_sessionID;

    ~DBConnector();

    auto lockPSQL() const -> std::unique_lock<std::mutex>;

    static auto getInstance() -> DBConnector&;
    auto init(const std::string& cryptoKey, const std::string& fileName) -> bool;

    PGconn* getConn() { return m_pConn; } // establish connection to database
    auto connectDB() -> bool;
    void disconnectDB();

    auto doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch = PGRES_COMMAND_OK, PGresult** ppRes = nullptr) -> bool;

    /*@ brief Inserts trade history into the table used to save the trading records. */
    auto insertTradingRecord(const TradingRecord& trade) -> bool;

protected:
    PGconn* m_pConn;
    mutable std::mutex m_mtxPSQL;

private:
    DBConnector(); // singleton pattern
    DBConnector(const DBConnector&) = delete; // forbid copying
    auto operator=(const DBConnector&) -> DBConnector& = delete; // forbid assigning

    std::unordered_map<std::string, std::string> m_loginInfo;
};
