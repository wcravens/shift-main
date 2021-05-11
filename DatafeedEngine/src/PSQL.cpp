#include "PSQL.h"

#include "FIXAcceptor.h"
#include "RawData.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <vector>

#include <shift/miscutils/Common.h>
#include <shift/miscutils/terminal/Common.h>

#define __DBG_DUMP_PQCMD false

void cvtRICToDEInternalRepresentation(std::string* pCvtThis, bool reverse /* = false */)
{
    const char from = reverse ? '_' : '.';
    const char to = reverse ? '.' : '_';
    std::replace(pCvtThis->begin(), pCvtThis->end(), from, to);
}

/**
 * @brief Generates Reuters-Time-Order series numbers (as a column data) so that they combined with the Reuters-Time column be used as primary key.
 *        To view the line-by-line correspondence of these two columns(i.e. primary key), use the SQL script:
 *          SELECT reuters_time, reuters_time_order
 *          FROM public.<a table name here>;
 */
/* static */ auto PSQL::s_getUpdateReutersTimeOrder(const std::string& currReutersTime, std::string* pLastRTime, int* pLastRTimeOrder) -> std::string
{
    if (currReutersTime.length() != pLastRTime->length() // Different size must implies different strings!
        /* Reuters time data are always in increasing order, usually with strides of some microseconds(10^-6s) to some milliseconds.
        *   This means that higher order time(e.g. Hours) are changing relatively slowly while lower one(e.g. Milliseconds) is changing frequently.
        *   Therefore, to efficiently compare the difference between adjacent Reuters Time strings, we shall use REVERSED comparison of these time strings.
        *   In this way it tends to more quickly detect the difference in each comparison.
        *  std::lexicographical_compare() lexicographically checks whether the 1st string is conceptually "less than" the 2nd one. */
        || std::lexicographical_compare(
            currReutersTime.rbegin(), currReutersTime.rend(),
            pLastRTime->rbegin(), pLastRTime->rend(),
            // comparator: Conceptually(i.e. not arithmetically) "Less than"; a "Equivalent to" b == !(a "Less than" b) && !(b "Less than" a).
            [](const std::string::value_type& lhs, const std::string::value_type& rhs) { return lhs != rhs; }) // currReutersTime "<" lastRTime ?
    ) {
        *pLastRTime = currReutersTime;
        *pLastRTimeOrder = 1;
        return "1";
    }

    return std::to_string(++*pLastRTimeOrder);
}

/* static */ inline auto PSQL::s_createTableName(const std::string& symbol, const std::string& yyyymmdd) -> std::string
{
    return shift::toLower(symbol) + '_' + yyyymmdd;
}

/* static */ inline auto PSQL::s_reutersDateToYYYYMMDD(const std::string& reutersDate) -> std::string
{
    return reutersDate.substr(0, 4) + reutersDate.substr(5, 2) + reutersDate.substr(8, 2);
}

//-----------------------------------------------------------------------------------------------

/**
 *
 * class PSQL
 * Note: PSQL is logically agnostic about the type conversion between FIX protocal data types and C++ standard types.
 *       All such conversion are encapsulated in other FIX-related parts(e.g. FIXAcceptor).
 *       It shall always use data types in DBFIXDataCarrier.h to transit data from/to FIX-related parts.
 */

PSQL::PSQL(std::unordered_map<std::string, std::string>&& loginInfo)
    : m_pConn { nullptr }
    , m_loginInfo { std::move(loginInfo) }
{
}

/*virtual*/ PSQL::~PSQL() /*= 0*/
{
    disconnectDB();
}

/**
 * @brief To provide a simpler syntax to lock.
 */
auto PSQL::lockPSQL() -> std::unique_lock<std::mutex>
{
    std::unique_lock<std::mutex> ul(m_mtxPSQL); // std::unique_lock is only move-able!
    return ul; // here it is MOVED(by C++11 mechanism) out in such situation, instead of being copied, hence ul's locked status will be preserved in caller side
}

/**
 * @brief Get login information.
 */
auto PSQL::getLoginInfo() const -> const std::unordered_map<std::string, std::string>&
{
    return m_loginInfo;
}

/**
 * @brief Establish connection to database.
 */
auto PSQL::connectDB() -> bool
{
    std::string info = "hostaddr=" + m_loginInfo["DBHost"] + " port=" + m_loginInfo["DBPort"] + " dbname=" + m_loginInfo["DBName"] + " user=" + m_loginInfo["DBUser"] + " password=" + m_loginInfo["DBPassword"];
    const char* c = info.c_str();
    auto lock { lockPSQL() };

    m_pConn = PQconnectdb(c);
    if (PQstatus(m_pConn) != CONNECTION_OK) {
        cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;

        PQfinish(m_pConn); // https://www.postgresql.org/docs/8.1/libpq.html \> PQfinish
        m_pConn = nullptr;

        return false;
    }

    return true;
}

/**
 * @brief Test connection to database.
 */
auto PSQL::isConnected() const -> bool
{
    return (nullptr != m_pConn) && (PQstatus(m_pConn) == CONNECTION_OK);
}

/**
 * @brief Close connection to database.
 */
void PSQL::disconnectDB()
{
    auto lock { lockPSQL() };
    if (nullptr == m_pConn) {
        return;
    }

    PQfinish(m_pConn);
    m_pConn = nullptr;
}

void PSQL::init()
{
    if (!connectDB()) {
        cout << COLOR_ERROR "ERROR: Cannot connect to database!" NO_COLOR << endl;
        cin.get();
        cin.get();
        return;
    }

    cout << "Connection to database is good." << endl;
}

auto PSQL::doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch /* = PGRES_COMMAND_OK */, PGresult** ppRes /* = nullptr */) -> bool
{
    return shift::database::doQuery(m_pConn, std::move(query), std::move(msgIfStatMismatch), statToMatch, ppRes);
}

/**
 * @brief Check if the Trade and Quote data for a specific Ric-ReutersDate combination is exist.
 *        If exist, saves the found table name to the parameter 'tableName'.
 */
auto PSQL::checkTableOfTradeAndQuoteRecordsExist(std::string ric, std::string reutersDate, std::string* pTableName) -> shift::database::TABLE_STATUS
{
    auto lock { lockPSQL() };
    using TABLE_STATUS = shift::database::TABLE_STATUS;

    auto tableName = PSQL::s_createTableName(ric, s_reutersDateToYYYYMMDD(reutersDate));

    PGresult* pRes = nullptr;
    if (!doQuery("SELECT EXISTS ("
                 "SELECT 1 "
                 "FROM pg_tables "
                 "WHERE schemaname = 'public' AND tablename = '"
                + tableName + "');",
            COLOR_ERROR "ERROR: SELECT EXISTS failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
        PQclear(pRes);
        return TABLE_STATUS::DB_ERROR;
    }

    int nrows = PQntuples(pRes);
    TABLE_STATUS status = TABLE_STATUS::OTHER_ERROR;

    if (1 == nrows) {
        char flag = *PQgetvalue(pRes, 0, 0);
        if (std::tolower(flag) == 't') {
            *pTableName = tableName;
            status = TABLE_STATUS::EXISTS;
        } else { // == 'f'
            *pTableName = "";
            status = TABLE_STATUS::NOT_EXIST;
        }
    }

    PQclear(pRes);

    return status;
}

/**
 * @brief Create Trade & Quote data table.
 */
auto PSQL::createTableOfTradeAndQuoteRecords(std::string tableName) -> bool
{
    auto lock { lockPSQL() };
    return doQuery("CREATE TABLE " + tableName + shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_colsDefinition, COLOR_ERROR "\tERROR: Create " + tableName + " table failed. (Please make sure that the old TAQ table was dropped.)\t" NO_COLOR);
}

/**
 * @brief Read csv file, Append statement and insert record into table.
 */
auto PSQL::insertTradeAndQuoteRecords(std::string csvName, std::string tableName) -> bool
{
    std::ifstream file(csvName); //define input stream
    std::string line;
    std::string cell;

    // to keep track ordering of reuters time so as to enable us create primary key (PK=(reuters_time, reuters_time_order))
    std::string lastRTime("N/A"); // **NOTE** : we assume that reuters time are always in increasing order in the CSV, otherwise the ordering algorithm here does NOT work!
    int lastRTimeOrder { 1 }; // initial value is always 1
    bool hasAnyData = false;

    // skip csv's headline
    std::getline(file, line);

    while (true) {
        std::string pqQuery = "INSERT INTO " + tableName + " (" + shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_recordFormat + ") VALUES ";
        size_t nVals = 0;

        while (nVals < 1e4 && std::getline(file, line)) { // reached one-time insertion limit?
            hasAnyData = true;
            // begin assembling a value:
            pqQuery += '(';

            std::stringstream lineStream(line);
            static auto readAppendField = [](auto& lineStream, auto& cell, auto& pqQuery, bool isNumeric) {
                std::getline(lineStream, cell, ',');
                if (cell.empty()) {
                    pqQuery += "NULL,";
                } else {
                    if (!isNumeric) {
                        pqQuery += '\'';
                    }
                    pqQuery += cell;
                    if (!isNumeric) {
                        pqQuery += '\'';
                    }
                    pqQuery += ',';
                }
            };

            // RIC
            readAppendField(lineStream, cell, pqQuery, false);

            // skip domain(?)
            std::getline(lineStream, cell, ',');

            // [PK] date-time & order
            std::getline(lineStream, cell, ','); // YYYY-MM-DDTHH:MM:SS.SSSSSSSSS(Z|-XX|+XX), where 'T' and 'Z' are required character literals
            {
                const auto dateTimeDelimPos = cell.find('T');
                const bool bGMTUTC = (cell.rfind('Z') != std::string::npos);
                const auto timeTailPos = bGMTUTC
                    ? cell.rfind('Z')
                    : (cell.substr(cell.rfind(':') + 1).rfind('-') != std::string::npos) // ensure there is '-' in the non-date parts for rfind('-')
                    ? cell.rfind('-') // expect "-XX"
                    : cell.rfind('+'); // expect "+XX"

                // reuters date
                pqQuery += '\'';
                pqQuery += cell.substr(0, dateTimeDelimPos);
                pqQuery += "','";

                // reuters time
                const auto& rtNanoSec = cell.substr(dateTimeDelimPos + 1, timeTailPos - dateTimeDelimPos - 1);
                const auto& rtMicroSec = rtNanoSec.substr({}, rtNanoSec.find('.') + 1 + 6); // truncated
                pqQuery += rtMicroSec;
                pqQuery += "',";

                // reuters time order (for primary key purpose)
                pqQuery += s_getUpdateReutersTimeOrder(rtMicroSec, &lastRTime, &lastRTimeOrder);
                pqQuery += ',';

                // reuters time offset
                if (bGMTUTC) {
                    pqQuery += "0,";
                } else if (std::string::npos == timeTailPos) { // no offset info
                    pqQuery += "NULL,";
                } else { // local exchange time
                    pqQuery += cell.substr(timeTailPos);
                    pqQuery += ',';
                }
            }

            pqQuery += '\'';
            // type: trade / quote
            std::getline(lineStream, cell, ',');
            bool isTrade = true;
            if ("Quote" == cell) {
                isTrade = false;
                pqQuery += 'Q';
            } else if ("Trade" == cell) {
                pqQuery += 'T';
            } else { // abnormal: missing/unknown type
                cout << " - " << COLOR_ERROR "ERROR: At least one Trade/Quote type cannot be determined! Insertion failed.\n" NO_COLOR;
                return false;
            }
            pqQuery += "',";

            // exchange id
            std::getline(lineStream, cell, ',');
            if (cell.length() > 0) {
                pqQuery += '\'';
                pqQuery += cell;
                pqQuery += "',";
            } else if (isTrade) {
                pqQuery += "'TRTH',"; // exchange_id: character varying(10)
            } else {
                pqQuery += "NULL,";
            }

            // price
            readAppendField(lineStream, cell, pqQuery, true);

            // volume
            readAppendField(lineStream, cell, pqQuery, true);

            // buyer id
            readAppendField(lineStream, cell, pqQuery, false);

            // bid price
            readAppendField(lineStream, cell, pqQuery, true);

            // bid size
            readAppendField(lineStream, cell, pqQuery, true);

            // seller id
            readAppendField(lineStream, cell, pqQuery, false);

            // ask price
            readAppendField(lineStream, cell, pqQuery, true);

            // ask size
            readAppendField(lineStream, cell, pqQuery, true);

            // exch & quote Time
            std::getline(lineStream, cell, ',');
            {
                const auto& etNanoSec = cell; // exch. time with praction in nano seconds
                // const auto fracPos = etNanoSec.find('.') + 1;
                // const auto& etMicroSec = etNanoSec.substr({}, etNanoSec.find('.') + 1 + 3); // trancated

                if (etNanoSec.empty()) { // Exch Time empty in CSV ?
                    pqQuery += "NULL,NULL";
                } else if (isTrade) {
                    // set SQL exch. time with blank quote time
                    pqQuery += '\'' + etNanoSec + "',NULL";
                } else {
                    // set SQL quote time with blank exch. time
                    pqQuery += "NULL,'" + etNanoSec + '\'';
                }
            }

            // finish assembling one value
            pqQuery += "),";

            ++nVals;
        } // while(getline)

        if (!hasAnyData) {
            cout << " - " << COLOR_WARNING "WARNING: " << csvName << " has no data to be inserted into database.\n" NO_COLOR;
            return true;
        }

        if (0 == nVals) { // finished processing all lines ?
            break;
        }

        pqQuery.back() = ';';

        auto lock { lockPSQL() };
        if (!doQuery(pqQuery, COLOR_ERROR "ERROR: Insert into [ " + tableName + " ] failed.\n" NO_COLOR)) {
            std::ofstream of("./PSQL_INSERT_ERROR_DUMP.txt");
            if (of.good()) {
                of.write(pqQuery.c_str(), pqQuery.length());
            }

            cout << COLOR_WARNING "The failed SQL query was written into './PSQL_INSERT_ERROR_DUMP.txt'. Please check it by manually executing the query. The table [" << tableName << "] was not created." NO_COLOR << endl;

            // DE should NOT keep any data of erroneous table, just discard them:
            doQuery("DROP TABLE " + tableName + " CASCADE;", "");

            return false;
        }
    } // while(true)

    return true;
}

/**
 * @brief Fetch chunk of Quote and trading records, and send to matching engine.
 */
auto PSQL::readSendRawData(std::string targetID, std::string symbol, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime) -> bool
{
    const std::string stime = boost::posix_time::to_iso_extended_string(startTime).substr(11, 8);
    const std::string etime = boost::posix_time::to_iso_extended_string(endTime).substr(11, 8);
    const std::string table_name = s_createTableName(symbol, boost::posix_time::to_iso_string(startTime).substr(0, 8)); /*YYYYMMDD*/
    auto lock { lockPSQL() };

    if (!doQuery("BEGIN", COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR)) {
        return false;
    }

    std::string pqQuery;
    pqQuery += "DECLARE data CURSOR FOR SELECT " + std::string(shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_recordFormat) + " FROM ";
    pqQuery += table_name;
    pqQuery += " WHERE reuters_time > '";
    pqQuery += stime;
    pqQuery += "' AND reuters_time <= '";
    pqQuery += etime;
    pqQuery += '\'';
    pqQuery += " ORDER BY reuters_time";

#if __DBG_DUMP_PQCMD
    cout << "DEBUG: pqQuery@readSendRawData ==\n\n"
         << pqQuery << '\n'
         << endl;
#endif

    if (!doQuery(pqQuery, COLOR_ERROR "ERROR: DECLARE CURSOR [ " + table_name + " ] failed. (send_raw_data)\n" NO_COLOR)) {
        return false;
    }

    PGresult* pRes = nullptr;
    if (!doQuery("FETCH ALL FROM data", COLOR_ERROR "ERROR: FETCH ALL failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
        PQclear(pRes);
        return false;
    }

    std::string queryTime;
    queryTime += "SELECT extract(epoch from reuters_date)::bigint, extract(epoch from reuters_time) FROM ";
    queryTime += table_name;
    queryTime += " WHERE reuters_time > '";
    queryTime += stime;
    queryTime += "' AND reuters_time <= '";
    queryTime += etime;
    queryTime += '\'';
    queryTime += " ORDER BY reuters_time";

    PGresult* pResTime = nullptr;
    if (!doQuery(queryTime, COLOR_ERROR "ERROR: EXTRACT TIME failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pResTime)) {
        PQclear(pResTime);
        return false;
    }

    double microsecs = 0.0;
    std::vector<RawData> rawData(PQntuples(pRes));
    std::setprecision(15);

    // next, save each record for each row into struct RawData and send to matching engine
    for (int i = 0, nt = PQntuples(pRes); i < nt; ++i) {
        char* pCh {};
        using VAL_IDX = shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::VAL_IDX;

        rawData[i].secs = std::atoi(PQgetvalue(pResTime, i, 0));
        sscanf(PQgetvalue(pResTime, i, 1), "%lf", &microsecs);
        rawData[i].microsecs = microsecs;

        rawData[i].symbol = PQgetvalue(pRes, i, VAL_IDX::RIC);
        rawData[i].reutersDate = PQgetvalue(pRes, i, VAL_IDX::REUT_DATE);
        rawData[i].reutersTime = PQgetvalue(pRes, i, VAL_IDX::REUT_TIME);
        rawData[i].toq = PQgetvalue(pRes, i, VAL_IDX::TOQ);
        rawData[i].exchangeID = PQgetvalue(pRes, i, VAL_IDX::EXCH_ID);
        rawData[i].price = std::strtod(PQgetvalue(pRes, i, VAL_IDX::PRICE), &pCh);
        rawData[i].volume = std::atoi(PQgetvalue(pRes, i, VAL_IDX::VOLUMN));
        rawData[i].buyerID = PQgetvalue(pRes, i, VAL_IDX::BUYER_ID);
        rawData[i].bidPrice = std::strtod(PQgetvalue(pRes, i, VAL_IDX::BID_PRICE), &pCh);
        rawData[i].bidSize = std::atoi(PQgetvalue(pRes, i, VAL_IDX::BID_SIZE));
        rawData[i].sellerID = PQgetvalue(pRes, i, VAL_IDX::SELLER_ID);
        rawData[i].askPrice = std::strtod(PQgetvalue(pRes, i, VAL_IDX::ASK_PRICE), &pCh);
        rawData[i].askSize = std::atoi(PQgetvalue(pRes, i, VAL_IDX::ASK_SIZE));
    }

    PQclear(pResTime);
    PQclear(pRes);

    pRes = PQexec(m_pConn, "CLOSE data");
    PQclear(pRes);
    pRes = PQexec(m_pConn, "END");
    PQclear(pRes);

    if (!rawData.empty()) {
        FIXAcceptor::sendRawData(targetID, rawData);
    }

    cout << std::setw(10 + 9) << table_name << std::setw(12) << std::right << " - Finished ";

    return true;
}

// /**
//  * @brief Check if Trade & Quote is empty or not.
//  */
// auto PSQL::checkEmpty(std::string tableName) -> long
// {
//     std::string pqQuery = "SELECT count(*) FROM " + tableName;
//     auto lock { lockPSQL() };

//     PGresult* pRes = PQexec(m_pConn, pqQuery.c_str());
//     if ('0' == *PQgetvalue(pRes, 0, 0)) {
//         PQclear(pRes);
//         return 0;
//     }
//     PQclear(pRes);

//     return 1;
// }

auto PSQL::saveCSVIntoDB(std::string csvName, std::string symbol, std::string date) -> bool // date format: YYYY-MM-DD
{
    const std::string tableName = s_createTableName(symbol, s_reutersDateToYYYYMMDD(date));

    cout << "Create table of [ " << symbol << ' ' << date << " ]";
    if (createTableOfTradeAndQuoteRecords(tableName)) {
        cout << " - OK." << endl;
    } else {
        cout << COLOR_ERROR " - Failed." NO_COLOR << endl;
        return false;
    }

    cout << "Insert records for [ " << symbol << ' ' << date << " ]";
    if (insertTradeAndQuoteRecords(csvName, tableName)) {
        cout << " - OK." << endl;
    } else {
        cout << COLOR_ERROR " - Failed." NO_COLOR << endl;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------------------

/**
 *
 * class PSQLManager
 *
 */

/* static */ PSQLManager* PSQLManager::s_pInst = nullptr;

PSQLManager::PSQLManager(std::unordered_map<std::string, std::string>&& loginInfo)
    : PSQL { std::move(loginInfo) }
{
}

/* static */ auto PSQLManager::createInstance(std::unordered_map<std::string, std::string>&& loginInfo) -> PSQLManager&
{
    static PSQLManager s_inst(std::move(loginInfo));
    s_pInst = &s_inst;
    return s_inst;
}

/* static */ auto PSQLManager::getInstance() -> PSQLManager&
{
    return *s_pInst;
}
