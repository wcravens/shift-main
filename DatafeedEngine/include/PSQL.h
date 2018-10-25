#pragma once

#include <quickfix/Application.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <postgresql/libpq-fe.h>

struct TradingRecord;

/**
 * @brief The PostgreSQL database manipulator
 */
class PSQL {
private:
    std::mutex m_mutex;
    PGconn* m_conn;
    std::unordered_map<std::string, std::string> m_loginInfo;
    static const std::string s_sessionID;

protected:
    PSQL(std::unordered_map<std::string, std::string>&& info);
    virtual ~PSQL() = 0; // PSQL becomes an abstract class, hence forces users to access it via PSQLManager

public:
    /*@brief Locker for SQL transactions. It also provides a simpler syntax to lock. */
    std::unique_lock<std::mutex> lockPSQL();

    auto getLoginInfo() const -> const decltype(m_loginInfo)& { return m_loginInfo; }

    PGconn* getConn() { return m_conn; }

    /*@brief Establish connection to database */
    bool connectDB();

    /*@brief Close connection to database */
    void disconnectDB();

    /*@brief Check the list of Trade-and-Quote-tables and the Table of Trade-Records */
    void init();

    /*@brief Indicates table status when querying */
    enum class TABLE_STATUS : int {
        NOT_EXIST = 0, // table does not exist
        EXISTS, // table exists
        DB_ERROR, // SQL database error
        OTHER_ERROR, // other error
    };

    /*@brief Check if specific table already exists */
    TABLE_STATUS checkTableExist(std::string tableName);

    /*@brief Create the table consists of all Trade & Quote Table names */
    bool createTableOfTableNames();

    /*@brief Insert one table name into the table created by createTableOfTableNames() */
    bool insertTableName(std::string ric, std::string reutersDate, std::string tableName);

    /*@brief Common PSQL query method */
    bool doQuery(const std::string query, const std::string msgIfNotOK);

    /*@brief Check if the Trade and Quote data for specific ric and date exist, and if so feeds back the table name */
    TABLE_STATUS checkTableOfTradeAndQuoteRecordsExist(std::string ric, std::string reutersDate, std::string& tableName);

    /*@brief Create new table of Trade & Quote data for one kind of RIC */
    bool createTableOfTradeAndQuoteRecords(std::string tableName);

    /*@brief Read .csv records data file and insert them into table created by createTableOfTradeAndQuoteRecords() */
    bool insertTradeAndQuoteRecords(std::string csvName, std::string tableName);

    /*@brief Fetch chunk of Trade & Quote records from database and sends them to matching engine via FIXAcceptor */
    bool readSendRawData(std::string symbol, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime, std::string targetID);

    /*@brief Check if Trade & Quote is empty */
    long checkEmpty(std::string tableName);

    /*@brief Create table used to save the trading records*/
    bool createTableOfTradingRecords();

    static std::string utc_to_string(const FIX::UtcTimeStamp& utc);

    /*@brief Inserts trade history into the table used to save the trading records*/
    bool insertTradingRecord(const TradingRecord& trade);

    /*@brief Convertor from CSV data to database records */
    bool saveCSVIntoDB(std::string csvName, std::string symbol, std::string date);
};

/**
 * @brief The organizer for PSQL singleton object. Users accessing to PostgreSQL shall always use it instead of bare PSQL.
 */
class PSQLManager final : public PSQL {
    static std::unique_ptr<PSQLManager> s_pInst;
    static std::once_flag s_instFlag;

    PSQLManager(std::unordered_map<std::string, std::string>&& loginInfo);

public:
    static PSQLManager& createInstance(std::unordered_map<std::string, std::string> dbLoginInfo);

    static PSQLManager& getInstance() { return *s_pInst; }

    PSQLManager(const PSQLManager&) = delete; // forbid copying
    void operator=(const PSQLManager&) = delete; // forbid assigning
};
