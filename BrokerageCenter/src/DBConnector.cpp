#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>

//----------------------------------------------------------------------------------------------------------------

/*static*/ bool DBConnector::s_isPortfolioDBReadOnly = false;

DBConnector::DBConnector()
    : m_pConn(nullptr)
{
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

    return shift::database::checkCreateTable<shift::database::BCTradingRecords>(m_pConn)
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
