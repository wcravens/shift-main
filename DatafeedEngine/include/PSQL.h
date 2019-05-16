#pragma once

#include <quickfix/Application.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <shift/miscutils/database/Common.h>

#include <postgresql/libpq-fe.h>

void cvtRICToDEInternalRepresentation(std::string* pCvtThis, bool reverse = false /*otherwise, from internal to RIC*/);

struct TradingRecord;

/**
 * @brief The PostgreSQL database manipulator
 */
class PSQL {
public:
    static std::string s_getUpdateReutersTimeOrder(const std::string& currReutersTime, std::string* pLastRTime, int* pLastRTimeOrder);

    static std::string s_createTableName(const std::string& symbol, const std::string& yyyymmdd);

    static std::string s_reutersDateToYYYYMMDD(const std::string& reutersDate);

    static double s_decimalTruncate(double value, int precision);

    /*@brief Locker for SQL transactions. It also provides a simpler syntax to lock. */
    std::unique_lock<std::mutex> lockPSQL();

    const std::unordered_map<std::string, std::string>& getLoginInfo() const { return m_loginInfo; }

    /*@brief Establish connection to database */
    bool connectDB();

    /*@brief Test connection to database */
    bool isConnected() const;

    /*@brief Close connection to database */
    void disconnectDB();

    /*@brief Check the list of Trade-and-Quote-tables and the Table of Trade-Records */
    void init();

    /*@brief Common PSQL query method wrapper */
    bool doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch = PGRES_COMMAND_OK, PGresult** ppRes = nullptr);

    /*@brief Check if the Trade and Quote data for specific ric and date exist, and if so feeds back the table name */
    shift::database::TABLE_STATUS checkTableOfTradeAndQuoteRecordsExist(std::string ric, std::string reutersDate, std::string* pTableName);

    /*@brief Create new table of Trade & Quote data for one kind of RIC */
    bool createTableOfTradeAndQuoteRecords(std::string tableName);

    /*@brief Read .csv records data file and insert them into table created by createTableOfTradeAndQuoteRecords() */
    bool insertTradeAndQuoteRecords(std::string csvName, std::string tableName);

    /*@brief Fetch chunk of Trade & Quote records from database and sends them to matching engine via FIXAcceptor */
    bool readSendRawData(std::string targetID, std::string symbol, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime);

    /*@brief Convertor from CSV data to database records */
    bool saveCSVIntoDB(std::string csvName, std::string symbol, std::string date);

protected:
    PSQL(std::unordered_map<std::string, std::string>&& info);
    virtual ~PSQL() = 0; // PSQL becomes an abstract class, hence forces users to access it via PSQLManager

private:
    std::mutex m_mtxPSQL; // To mutual-exclusively access db
    PGconn* m_pConn;
    std::unordered_map<std::string, std::string> m_loginInfo;
};

/**
 * @brief The organizer for PSQL singleton object. Users accessing to PostgreSQL shall always use it instead of bare PSQL.
 */
class PSQLManager final : public PSQL {
public:
    PSQLManager(const PSQLManager&) = delete; // forbid copying
    void operator=(const PSQLManager&) = delete; // forbid assigning

    static PSQLManager& createInstance(std::unordered_map<std::string, std::string>&& dbLoginInfo);
    static PSQLManager& getInstance() { return *s_pInst; }

private:
    PSQLManager(std::unordered_map<std::string, std::string>&& loginInfo);

    static PSQLManager* s_pInst;
};
