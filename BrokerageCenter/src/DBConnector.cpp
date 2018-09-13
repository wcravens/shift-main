#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>

/*static*/ DBConnector* DBConnector::instance()
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
 * @brief   Establish connection to database 
 * @return  whether the connection had been built
 */
bool DBConnector::connectDB()
{
    disconnectDB();

    std::string info = "hostaddr=" + m_loginInfo["DBHostaddr"] + " port=" + m_loginInfo["DBPort"] + " dbname=" + m_loginInfo["DBname"] + " user=" + m_loginInfo["DBUser"] + " password=" + m_loginInfo["DBPassword"];
    // Make a connection to the database
    const char* c = info.c_str();
    m_pConn = PQconnectdb(c);

    // Check to see if connection was successfully made
    if (PQstatus(m_pConn) == CONNECTION_OK) {
        return true;
    }

    // fail to connect the database
    disconnectDB();
    cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;
    return false;
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

/**
 * @brief   
 */
bool DBConnector::createClients(const std::string& symbol)
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
            std::string clientID = PQgetvalue(res1, i, 0);
            auto buyingPower = atof(PQgetvalue(res1, i, 1));
            auto shares = atoi(PQgetvalue(res2, i, 1));
            auto price = atof(PQgetvalue(res2, i, 2));

            BCDocuments::instance()->addRiskManagementToClient(clientID, buyingPower, shares, price, symbol);
            cout << clientID << endl;
        }
    }

    PQclear(res2);
    PQclear(res1);
    return res;
}

//-------------------------------------------------------------------

std::vector<std::string> readCol(const std::string& sql)
{
    PGresult* res = PQexec(DBConnector::instance()->getConn(), sql.c_str());
    std::vector<std::string> vs;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cout << COLOR_ERROR "ERROR: Get failed.\n" NO_COLOR;
    } else {
        int nrows = PQntuples(res);
        for (int i = 0; i < nrows; i++) {
            vs.push_back(PQgetvalue(res, i, 0));
        }
    }

    PQclear(res);
    return vs;
}

// void DBConnector::InsertUser(const UserInfo& client)
// {
//     std::string pqQuery;
//     pqQuery = "INSERT INTO client_information VALUES ('" + client.id + "','" + client.fistName + "','" + client.lastName + "','" + client.accountName + "','" + client.password + "','" + std::to_string(client.buyingPower) + "','" + client.email + "','" + std::to_string(int(client.gender)) + "');";

//     PGresult* res = PQexec(m_pConn, pqQuery.c_str());

//     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
//         cout << "Error. Insert to qt_tablelist table failed.\n";
//         PQclear(res);
//     } else {
//         PQclear(res);
//         cout << "Successful Inserted!";
//     }
// }
