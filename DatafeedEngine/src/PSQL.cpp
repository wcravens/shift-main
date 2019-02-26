#include "PSQL.h"

#include "DBFIXDataCarrier.h"
#include "FIXAcceptor.h"

#include <ctime>
#include <iomanip>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

#define __DBG_DUMP_PQCMD false

static inline std::string createTableName(const std::string& symbol, const std::string& yyyymmdd)
{
    return symbol + '_' + yyyymmdd;
}

/**
 * @brief Generates Reuters-Time-Order series numbers (as a column data) so that they combined with the Reuters-Time column be used as primary key.
 *        To view the line-by-line correspondence of these two columns(i.e. primary key), use the SQL script:
 *          SELECT reuters_time, reuters_time_order
 *          FROM public.<a table name here>;
 */
static std::string getUpdateReutersTimeOrder(const std::string& currReutersTime, std::string& lastRTime, int& lastRTimeOrder)
{
    if (currReutersTime.length() != lastRTime.length() // Different size must implies different strings!
        /* Reuters time data are always in increasing order, usually with strides of some microseconds(10^-6s) to some milliseconds.
        *   This means that higher order time(e.g. Hours) are changing relatively slowly while lower one(e.g. Milliseconds) is changing frequently.
        *   Therefore, to efficiently compare the difference between adjacent Reuters Time strings, we shall use REVERSED comparison of these time strings.
        *   In this way it tends to more quickly detect the difference in each comparison.
        *  std::lexicographical_compare() lexicographically checks whether the 1st string is conceptually "less than" the 2nd one. */
        || std::lexicographical_compare(
               currReutersTime.rbegin(), currReutersTime.rend(),
               lastRTime.rbegin(), lastRTime.rend(),
               // comparator: Conceptually(i.e. not arithmetically) "Less than"; a "Equivalent to" b == !(a "Less than" b) && !(b "Less than" a).
               [](const std::string::value_type& lhs, const std::string::value_type& rhs) { return lhs != rhs; }) // currReutersTime "<" lastRTime ?
    ) {
        lastRTime = currReutersTime;
        lastRTimeOrder = 1;
        return "1";
    }

    return std::to_string(++lastRTimeOrder);
}

//-----------------------------------------------------------------------------------------------

/**
 *
 * class PSQL
 * Note: PSQL is logically agnostic about the type conversion between FIX protocal data types and C++ standard types.
 *       All such conversion are encapsulated in other FIX-related parts(e.g. FIXAcceptor).
 *       It shall always use data types in DBFIXDataCarrier.h to transit data from/to FIX-related parts.
 */

/*static*/ const std::string PSQL::s_sessionID = shift::crossguid::newGuid().str();

PSQL::PSQL(std::unordered_map<std::string, std::string>&& info)
    : m_loginInfo(std::move(info))
{
    cout << "Session ID: " << s_sessionID << endl;
}

/*virtual*/ PSQL::~PSQL() /*= 0*/
{
    disconnectDB();
}

/**
 * @brief To provide a simpler syntax to lock.
 */
std::unique_lock<std::mutex> PSQL::lockPSQL()
{
    std::unique_lock<std::mutex> ul(m_mutex); // std::unique_lock is only move-able!
    return ul; // here it is MOVED(by C++11 mechanism) out in such situation, instead of being copied, hence ul's locked status will be preserved in caller side
}

/* Establish connection to database */
bool PSQL::connectDB()
{
    std::string info = "hostaddr=" + m_loginInfo["DBHostaddr"] + " port=" + m_loginInfo["DBPort"] + " dbname=" + m_loginInfo["DBname"] + " user=" + m_loginInfo["DBUser"] + " password=" + m_loginInfo["DBPassword"];
    const char* c = info.c_str();
    auto lock{ lockPSQL() };

    m_conn = PQconnectdb(c);
    if (PQstatus(m_conn) != CONNECTION_OK) {
        cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;

        PQfinish(m_conn); // https://www.postgresql.org/docs/8.1/libpq.html \> PQfinish
        m_conn = nullptr;

        return false;
    }

    return true;
}

bool PSQL::isConnected() const
{
    return m_conn && PQstatus(m_conn) == CONNECTION_OK;
}

/* Close connection to database */
void PSQL::disconnectDB()
{
    auto lock{ lockPSQL() };
    if (nullptr == m_conn)
        return;

    PQfinish(m_conn);
    m_conn = nullptr;
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

    auto res = shift::database::checkCreateTable<shift::database::NamesOfTradeAndQuoteTables>(m_conn)
        && shift::database::checkCreateTable<shift::database::DETradingRecords>(m_conn);
    (void)res;
}

bool PSQL::doQuery(std::string query, const std::string msgIfStatMismatch, ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
{
    return shift::database::doQuery(m_conn, std::move(query), msgIfStatMismatch, statToMatch, ppRes);
}

/* Insert record to the table after updating one day quote&trade data to the database*/
bool PSQL::insertTableName(std::string ric, std::string reutersDate, std::string tableName)
{
    auto lock{ lockPSQL() };
    return doQuery("INSERT INTO " + std::string(shift::database::PSQLTable<shift::database::NamesOfTradeAndQuoteTables>::name) + " VALUES ('" + ric + "','" + reutersDate + "','" + tableName + "');", COLOR_ERROR "\tERROR: Insert into " + std::string(shift::database::PSQLTable<shift::database::NamesOfTradeAndQuoteTables>::name) + " table failed.\t" NO_COLOR);
}

/* Create Trade & Quote data table */
bool PSQL::createTableOfTradeAndQuoteRecords(std::string tableName)
{
    auto lock{ lockPSQL() };
    return doQuery("CREATE TABLE " + tableName + shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_colsDefinition, COLOR_ERROR "\tERROR: Create " + tableName + " table failed. (Please make sure that the old TAQ table was dropped.)\t" NO_COLOR);
}

/* Check if the Trade and Quote data for specific ric and date is exist
    save the table name in the std::string tableName                    */
shift::database::TABLE_STATUS PSQL::checkTableOfTradeAndQuoteRecordsExist(std::string ric, std::string reutersDate, std::string& tableName)
{
    auto lock{ lockPSQL() };
    using TABLE_STATUS = shift::database::TABLE_STATUS;

    if (!doQuery("BEGIN", COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR))
        return TABLE_STATUS::DB_ERROR;

    if (!doQuery("DECLARE record CURSOR FOR SELECT * FROM " + std::string(shift::database::PSQLTable<shift::database::NamesOfTradeAndQuoteTables>::name) + " WHERE reuters_date='" + reutersDate + "' AND ric='" + ric + '\'', COLOR_ERROR "ERROR: DECLARE CURSOR failed. (check_taq_tbl_exist)\n" NO_COLOR))
        return TABLE_STATUS::DB_ERROR;

    PGresult* pRes;
    if (!doQuery("FETCH ALL IN record", COLOR_ERROR "ERROR: FETCH ALL failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
        PQclear(pRes);
        return TABLE_STATUS::DB_ERROR;
    }

    int nrows = PQntuples(pRes);
    TABLE_STATUS status;

    if (0 == nrows) {
        status = TABLE_STATUS::NOT_EXIST;
    } else if (1 == nrows) {
        tableName = PQgetvalue(pRes, 0, shift::database::PSQLTable<shift::database::NamesOfTradeAndQuoteTables>::VAL_IDX::REUT_TABLE_NAME);
        status = TABLE_STATUS::EXISTS;
    } else {
        cout << COLOR_ERROR "ERROR: More than one Trade & Quote table for [ " << ric << ' ' << reutersDate << " ] exist." NO_COLOR << endl;
        status = TABLE_STATUS::OTHER_ERROR;
    }

    PQclear(pRes);
    pRes = PQexec(m_conn, "CLOSE record");
    PQclear(pRes);
    pRes = PQexec(m_conn, "END");
    PQclear(pRes);

    return status;
}

/* read csv file, Append statement and insert record into table */
bool PSQL::insertTradeAndQuoteRecords(std::string csvName, std::string tableName)
{
    std::ifstream file(csvName); //define input stream
    std::string line;
    std::string cell;

    // to keep track ordering of reuters time so as to enable us create primary key (PK=(reuters_time, reuters_time_order)).
    std::string lastRTime("N/A"); // **NOTE** : We assume that reuters time are always in increasing order in the CSV, otherwise the ordering algorithm here does NOT work!
    int lastRTimeOrder{ 1 }; // initial value is always 1
    bool hasAnyData = false;

    // skip csv's headline
    std::getline(file, line);

    while (true) {
        std::string pqQuery = "INSERT INTO " + tableName + shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_recordFormat;
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
                    if (!isNumeric)
                        pqQuery += '\'';
                    pqQuery += cell;
                    if (!isNumeric)
                        pqQuery += '\'';
                    pqQuery += ',';
                }
            };

            // RIC
            readAppendField(lineStream, cell, pqQuery, false);

            // skip Domain(?)
            std::getline(lineStream, cell, ',');

            // [PK] Date-Time & order
            std::getline(lineStream, cell, ','); // YYYY-MM-DDTHH:MM:SS.SSSSSSSSS(Z|-XX|+XX), where T and Z are required character literals
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
                pqQuery += ::getUpdateReutersTimeOrder(rtMicroSec, lastRTime, lastRTimeOrder);
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
            // type: Trade / Quote
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
            if (cell.length()) {
                pqQuery += '\'';
                pqQuery += cell;
                pqQuery += "',";
            } else if (isTrade) {
                pqQuery += "'TRADE_EXCH_ID',";
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

                if (etNanoSec.empty()) // Exch Time empty in CSV ?
                    pqQuery += "NULL,NULL";
                else if (isTrade) {
                    // set SQL exch. time with blank quote time
                    pqQuery += '\'' + etNanoSec + "',NULL";
                } else {
                    // set SQL quote time with blank exch. time
                    pqQuery += "NULL,'" + etNanoSec + '\'';
                }
            }

            // finish assembling one value
            pqQuery += "),";

            nVals++;
        } // while(getline)

        if (!hasAnyData) {
            cout << " - " << COLOR_WARNING "WARNING: " << csvName << " has no data to be inserted into database.\n" NO_COLOR;
            return true;
        }

        if (0 == nVals) // finished processing all lines ?
            break;

        pqQuery.back() = ';';

        auto lock{ lockPSQL() };
        if (!doQuery(pqQuery, COLOR_ERROR "ERROR: Insert into [ " + tableName + " ] failed.\n" NO_COLOR)) {
            std::ofstream of("./PSQL_INSERT_ERROR_DUMP.txt");
            if (of.good())
                of.write(pqQuery.c_str(), pqQuery.length());
            cout << COLOR_WARNING "The failed SQL query was written into ./PSQL_INSERT_ERROR_DUMP.txt. Please check it by manually executing the query." NO_COLOR << endl;
            return false;
        }
    } // while(true)

    return true;
}

/* Fetch chunk of Quote and trading records, and send to matching engine*/
bool PSQL::readSendRawData(std::string symbol, boost::posix_time::ptime startTime, boost::posix_time::ptime endTime, std::string targetID)
{
    const std::string stime = boost::posix_time::to_iso_extended_string(startTime).substr(11, 8);
    const std::string etime = boost::posix_time::to_iso_extended_string(endTime).substr(11, 8);
    const std::string table_name = ::createTableName(symbol, boost::posix_time::to_iso_string(startTime).substr(0, 8)); /*YYYYMMDD*/
    auto lock{ lockPSQL() };

    if (!doQuery("BEGIN", COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR))
        return false;

    const std::string csTQRecFmt = shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::sc_recordFormat;
    std::string pqQuery;
    pqQuery += "DECLARE data CURSOR FOR SELECT " + csTQRecFmt.substr(csTQRecFmt.find('(') + 1, csTQRecFmt.rfind(')') - csTQRecFmt.find('(') - 1) + " FROM ";
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

    if (!doQuery(pqQuery, COLOR_ERROR "ERROR: DECLARE CURSOR [ " + table_name + " ] failed. (send_raw_data)\n" NO_COLOR))
        return false;

    PGresult* pRes;
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

    PGresult* pResTime;
    if (!doQuery(queryTime, COLOR_ERROR "ERROR: EXTRACT TIME failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pResTime)) {
        PQclear(pResTime);
        return false;
    }

    double microsecs;
    RawData rawData;
    std::setprecision(15);

    // Next, save each record for each row into struct RawData and send to matching engine
    for (int i = 0, nt = PQntuples(pRes); i < nt; i++) {
        char* pCh{};
        using VAL_IDX = shift::database::PSQLTable<shift::database::TradeAndQuoteRecords>::VAL_IDX;

        rawData.secs = std::atol(PQgetvalue(pResTime, i, 0));
        sscanf(PQgetvalue(pResTime, i, 1), "%lf", &microsecs);
        rawData.microsecs = microsecs;

        rawData.symbol = PQgetvalue(pRes, i, VAL_IDX::RIC);
        rawData.reutersDate = PQgetvalue(pRes, i, VAL_IDX::REUT_DATE);
        rawData.reutersTime = PQgetvalue(pRes, i, VAL_IDX::REUT_TIME);
        rawData.toq = PQgetvalue(pRes, i, VAL_IDX::TOQ);
        rawData.exchangeID = PQgetvalue(pRes, i, VAL_IDX::EXCH_ID);
        rawData.price = std::strtod(PQgetvalue(pRes, i, VAL_IDX::PRICE), &pCh);
        rawData.volume = std::atoi(PQgetvalue(pRes, i, VAL_IDX::VOLUMN));
        rawData.buyerID = PQgetvalue(pRes, i, VAL_IDX::BUYER_ID);
        rawData.bidPrice = std::strtod(PQgetvalue(pRes, i, VAL_IDX::BID_PRICE), &pCh);
        rawData.bidSize = std::atoi(PQgetvalue(pRes, i, VAL_IDX::BID_SIZE));
        rawData.sellerID = PQgetvalue(pRes, i, VAL_IDX::SELLER_ID);
        rawData.askPrice = std::strtod(PQgetvalue(pRes, i, VAL_IDX::ASK_PRICE), &pCh);
        rawData.askSize = std::atoi(PQgetvalue(pRes, i, VAL_IDX::ASK_SIZE));

        FIXAcceptor::sendRawData(rawData, targetID);
    }

    PQclear(pResTime);
    PQclear(pRes);

    pRes = PQexec(m_conn, "CLOSE data");
    PQclear(pRes);
    pRes = PQexec(m_conn, "END");
    PQclear(pRes);

    cout << std::setw(10 + 9) << table_name << std::setw(12) << std::right << " - Finished ";

    return true;
}

// /* Check if Trade & Quote is empty or not */
// long PSQL::checkEmpty(std::string tableName)
// {
//     std::string pqQuery = "SELECT count(*) FROM " + tableName;
//     auto lock{ lockPSQL() };

//     PGresult* pRes = PQexec(m_conn, pqQuery.c_str());
//     if ('0' == *PQgetvalue(pRes, 0, 0)) {
//         PQclear(pRes);
//         return 0;
//     }
//     PQclear(pRes);

//     return 1;
// }

/**
 * @brief Convert FIX::UtcTimeStamp to string used for date field in DB
 */
/* static */ std::string PSQL::utcToString(const FIX::UtcTimeStamp& ts, bool localTime)
{
    std::ostringstream os;
    time_t t = ts.getTimeT();
    if (localTime) {
        os << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    } else {
        os << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S");
    }
    os << '.';
    os << ts.getFraction(6);
    return os.str();
}

bool PSQL::insertTradingRecord(const TradingRecord& trade)
{
    std::ostringstream temp;
    temp << "INSERT INTO " << shift::database::PSQLTable<shift::database::DETradingRecords>::name << " VALUES ('"
         << s_sessionID << "','"
         << utcToString(trade.realtime, true) << "','"
         << utcToString(trade.exetime, true) << "','"
         << trade.symbol << "','"
         << trade.price << "','"
         << trade.size << "','"
         << trade.trader_id_1 << "','"
         << trade.trader_id_2 << "','"
         << trade.order_id_1 << "','"
         << trade.order_id_2 << "','"
         << trade.order_type_1 << "','"
         << trade.order_type_2 << "','"
         << utcToString(trade.time1, true) << "','"
         << utcToString(trade.time2, true) << "','"
         << trade.decision << "','"
         << trade.destination << "');";

    auto lock{ lockPSQL() };
    if (!doQuery(temp.str(), COLOR_WARNING "Insert into [ " + std::string(shift::database::PSQLTable<shift::database::DETradingRecords>::name) + " ] failed.\n" NO_COLOR))
        return false;

    return true;
}

bool PSQL::saveCSVIntoDB(std::string csvName, std::string symbol, std::string date) // Date format: YYYY-MM-DD
{
    std::string tableName = ::createTableName(symbol, date.substr(0, 4) + date.substr(5, 2) + date.substr(8, 2));

    cout << "Insert [ " << symbol << ' ' << date << " ] into table name list";
    if (insertTableName(symbol, date, tableName)) {
        cout << " - OK." << endl;
    } else {
        cout << COLOR_ERROR " - Failed." NO_COLOR << endl;
        return false;
    }

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

/*static*/ PSQLManager* PSQLManager::s_pInst = nullptr;

PSQLManager::PSQLManager(std::unordered_map<std::string, std::string>&& dbInfo)
    : PSQL(std::move(dbInfo))
{
}

/*static*/ PSQLManager& PSQLManager::createInstance(std::unordered_map<std::string, std::string> dbLoginInfo)
{
    static PSQLManager s_inst(std::move(dbLoginInfo));
    s_pInst = &s_inst;
    return s_inst;
}
