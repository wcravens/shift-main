#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <libpq-fe.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <quickfix/Application.h>

#include <shift/miscutils/database/Common.h>

void cvtRICToDEInternalRepresentation(std::string* pCvtThis, bool reverse = false /*otherwise, from internal to RIC*/);

struct TradingRecord;

/**
 * @brief The PostgreSQL database manipulator.
 */
class PSQL {
public:
    static auto s_getUpdateReutersTimeOrder(const std::string& currReutersTime, std::string* pLastRTime, int* pLastRTimeOrder) -> std::string;

    static auto s_createTableName(const std::string& symbol, const std::string& yyyymmdd) -> std::string;

    static auto s_reutersDateToYYYYMMDD(const std::string& reutersDate) -> std::string;

    /* @brief Locker for SQL transactions. It also provides a simpler syntax to lock. */
    auto lockPSQL() -> std::unique_lock<std::mutex>;

    /* @brief Get login information. */
    auto getLoginInfo() const -> const std::unordered_map<std::string, std::string>&;

    /* @brief Establish connection to database. */
    auto connectDB() -> bool;

    /* @brief Test connection to database. */
    auto isConnected() const -> bool;

    /* @brief Close connection to database. */
    void disconnectDB();

    /* @brief Check the list of Trade-and-Quote-tables and the Table of Trade-Records. */
    void init();

    /* @brief Common PSQL query method wrapper. */
    auto doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch = PGRES_COMMAND_OK, PGresult** ppRes = nullptr) -> bool;

    /* @brief Check if the Trade and Quote data for specific ric and date exist, and if so feeds back the table name. */
    auto checkTableOfTradeAndQuoteRecordsExist(std::string ric, std::string reutersDate, std::string* pTableName) -> shift::database::TABLE_STATUS;

    /* @brief Create new table of Trade & Quote data for one kind of RIC. */
    auto createTableOfTradeAndQuoteRecords(std::string tableName) -> bool;

    /* @brief Read .csv records data file and insert them into table created by createTableOfTradeAndQuoteRecords(). */
    auto insertTradeAndQuoteRecords(std::string csvName, std::string tableName) -> bool;

    /* @brief Fetch chunk of Trade & Quote records from database and sends them to matching engine via FIXAcceptor. */
    auto readSendRawData(std::string targetID, std::string symbol, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime) -> bool;

    /* @brief Convertor from CSV data to database records. */
    auto saveCSVIntoDB(std::string csvName, std::string symbol, std::string date) -> bool;

protected:
    PSQL(std::unordered_map<std::string, std::string>&& loginInfo);
    virtual ~PSQL() = 0; // PSQL becomes an abstract class, hence forces users to access it via PSQLManager

private:
    std::mutex m_mtxPSQL; // to mutual-exclusively access db
    PGconn* m_pConn;
    std::unordered_map<std::string, std::string> m_loginInfo;
};

/**
 * @brief The organizer for PSQL singleton object. Users accessing to PostgreSQL shall always use it instead of bare PSQL.
 */
class PSQLManager final : public PSQL {
public:
    static auto createInstance(std::unordered_map<std::string, std::string>&& loginInfo) -> PSQLManager&;
    static auto getInstance() -> PSQLManager&;

private:
    PSQLManager(std::unordered_map<std::string, std::string>&& loginInfo); // singleton pattern
    PSQLManager(const PSQLManager&) = delete; // forbid copying
    auto operator=(const PSQLManager&) -> PSQLManager& = delete; // forbid assigning

    static PSQLManager* s_pInst;
};
