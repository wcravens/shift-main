#include "DBConnector.h"

#include <sstream>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>

//----------------------------------------------------------------------------------------------------------------

/* static */ bool DBConnector::s_hasConnected = false;
/* static */ const std::string DBConnector::s_sessionID = shift::crossguid::newGuid().str();

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

/* static */ DBConnector* DBConnector::getInstance()
{
    static DBConnector s_DBInst;
    return &s_DBInst;
}

bool DBConnector::init(const std::string& cryptoKey, const std::string& fileName)
{
    m_loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, fileName);
    cout << "READING:" << fileName << endl;

    return m_loginInfo.size();
}

/**
 * @brief Establish connection to database.
 * @return Whether the connection had been built.
 */
bool DBConnector::connectDB()
{
    disconnectDB();

    std::string info = "hostaddr=" + m_loginInfo["DBHost"] + " port=" + m_loginInfo["DBPort"] + " dbname=" + m_loginInfo["DBName"] + " user=" + m_loginInfo["DBUser"] + " password=" + m_loginInfo["DBPassword"];
    m_pConn = PQconnectdb(info.c_str());
    if (PQstatus(m_pConn) != CONNECTION_OK) {
        disconnectDB();
        cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;
        return false;
    } else {
        cout << "CONNECTION IS A-OK" << endl;
        DBConnector::s_hasConnected = true;
    }

    return DBConnector::s_hasConnected;
}

/**
 * @brief Close connection to database.
 */
void DBConnector::disconnectDB()
{
    if (!m_pConn)
        return;

    PQfinish(m_pConn);
    m_pConn = nullptr;
}

/**
 * @brief: Used to issue queries. IE: CREATE, BEGIN ,SELECT etc.
 * @param pConn: psql connection
 * @param query: string query
 * @param msgIfStatMismatch: string, to print if the query failed
 * @param statToMatch: psql status condition expected.
 *        @NOTE: If you want to execute a result that returns tuples, use the flag PGRES_TUPLES_OK
 * @param ppRes: double pointer to the target PGresult object. call like (&pRes)
 */
bool DBConnector::doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
{
    return shift::database::doQuery(m_pConn, std::move(query), std::move(msgIfStatMismatch), statToMatch, ppRes);
}

std::vector<std::string> DBConnector::readRowsOfField(std::string query, int fieldIndex /*= 0*/)
{
    return shift::database::readRowsOfField(m_pConn, std::move(query), fieldIndex);
}
