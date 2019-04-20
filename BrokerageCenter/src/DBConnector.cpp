#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>
#include <sstream>

//----------------------------------------------------------------------------------------------------------------

/*static*/ bool DBConnector::s_isPortfolioDBReadOnly = false;
/*static*/ const std::string DBConnector::s_sessionID = shift::crossguid::newGuid().str();

DBConnector::DBConnector()
    : m_pConn(nullptr)
{
    cout << "\nSession ID: " << s_sessionID << '\n'
         << endl;
}

DBConnector::~DBConnector()
{
    disconnectDB();
}

/**
 * @brief To provide a simpler syntax to lock.
 */
std::unique_lock<std::mutex> DBConnector::lockPSQL() const
{
    std::unique_lock<std::mutex> lock(m_mtxPSQL);
    return lock; // move()-ed out here
}

/*static*/ DBConnector* DBConnector::getInstance()
{
    static DBConnector s_DBInst;
    return &s_DBInst;
}

bool DBConnector::init(const std::string& cryptoKey, const std::string& fileName)
{
    m_loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, fileName);
    return m_loginInfo.size();
}

/**
 * @brief   Establish connection to database
 * @return  whether the connection had been built
 */
bool DBConnector::connectDB()
{
    disconnectDB();

    std::string info = "hostaddr=" + m_loginInfo["DBHostaddr"] + " port=" + m_loginInfo["DBPort"] + " dbname=" + m_loginInfo["DBname"] + " user=" + m_loginInfo["DBUser"] + " password=" + m_loginInfo["DBPassword"];
    m_pConn = PQconnectdb(info.c_str());
    if (PQstatus(m_pConn) != CONNECTION_OK) {
        disconnectDB();
        cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;
        return false;
    }

    return shift::database::checkCreateTable<shift::database::TradingRecords>(m_pConn)
        && shift::database::checkCreateTable<shift::database::PortfolioSummary>(m_pConn)
        && shift::database::checkCreateTable<shift::database::PortfolioItem>(m_pConn);
}

/**
 * @brief   Close connection to database
 */
void DBConnector::disconnectDB()
{
    if (!m_pConn)
        return;

    PQfinish(m_pConn);
    m_pConn = nullptr;
}

bool DBConnector::doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
{
    return shift::database::doQuery(m_pConn, std::move(query), std::move(msgIfStatMismatch), statToMatch, ppRes);
}

/**
 * @brief Convert FIX::UtcTimeStamp to string used for date field in DB
 */
static std::string s_utcToString(const FIX::UtcTimeStamp& ts, bool localTime)
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

bool DBConnector::insertTradingRecord(const TradingRecord& trade)
{
    std::ostringstream queryStrm;
    queryStrm << "INSERT INTO " << shift::database::PSQLTable<shift::database::TradingRecords>::name << " VALUES ('"
              << s_sessionID << "','"
              << s_utcToString(trade.realTime, true) << "','"
              << s_utcToString(trade.simulationTime, true) << "','"
              << trade.symbol << "','"
              << trade.price << "','"
              << trade.size << "','"
              << trade.traderID1 << "','"
              << trade.traderID2 << "','"
              << trade.orderID1 << "','"
              << trade.orderID2 << "','"
              << static_cast<char>(trade.orderType1) << "','"
              << static_cast<char>(trade.orderType2) << "','"
              << s_utcToString(trade.simulationTime1, true) << "','"
              << s_utcToString(trade.simulationTime2, true) << "','"
              << trade.decision << "','"
              << trade.destination << "');";

    auto lock{ lockPSQL() };
    if (!doQuery(queryStrm.str(), COLOR_ERROR "ERROR: Insert into [" + std::string(shift::database::PSQLTable<shift::database::TradingRecords>::name) + "] failed with query:\n" NO_COLOR)) {
        cout << COLOR_WARNING << queryStrm.str() << NO_COLOR << '\n'
             << endl;
        return false;
    }
    return true;
}