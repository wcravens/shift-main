#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/database/Common.h>
#include <shift/miscutils/terminal/Common.h>

//----------------------------------------------------------------------------------------------------------------

/*static*/ bool DBConnector::s_isPortfolioDBReadOnly = false;

/*static*/ DBConnector* DBConnector::getInstance()
{
    static DBConnector s_DBInst;
    return &s_DBInst;
}

DBConnector::DBConnector()
    : m_pConn(nullptr)
{
}

bool DBConnector::init(const std::string& cryptoKey, const std::string& fileName)
{
    m_loginInfo = shift::crypto::readEncryptedConfigFile(cryptoKey, fileName);
    return m_loginInfo.size();
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

bool DBConnector::createUsers(const std::string& symbol)
{
    PGresult* res1 = PQexec(m_pConn, "SELECT * FROM buying_power;");
    PGresult* res2 = PQexec(m_pConn, ("SELECT * FROM holdings_" + symbol + ";").c_str());
    bool res = true;

    if ((PQresultStatus(res1) != PGRES_TUPLES_OK) || (PQresultStatus(res2) != PGRES_TUPLES_OK)) {
        cout << PQerrorMessage(m_pConn) << endl;
        res = false;
    } else {
        auto nTup = PQntuples(res1);

        cout << "buying_power: " << nTup << " " << PQnfields(res1) << endl;
        cout << "holdings_" + symbol + ": " << PQnfields(res2) << endl;

        for (int i = 0; i < nTup; i++) {
            std::string username = PQgetvalue(res1, i, 0);
            auto buyingPower = atof(PQgetvalue(res1, i, 1));
            auto shares = atoi(PQgetvalue(res2, i, 1));
            auto price = atof(PQgetvalue(res2, i, 2));

            BCDocuments::getInstance()->addRiskManagementToUserLockedExplicit(username, buyingPower, shares, price, symbol);
            cout << username << '\n';
        }
        cout << endl;
    }

    PQclear(res2);
    PQclear(res1);
    return res;
}

bool DBConnector::doQuery(std::string query, std::string msgIfStatMismatch, ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
{
    return shift::database::doQuery(m_pConn, std::move(query), std::move(msgIfStatMismatch), statToMatch, ppRes);
}
