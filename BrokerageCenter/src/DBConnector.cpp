#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>

#define CSTR_TBLNAME_TRADING_RECORDS "trading_records"

struct TradingRecords;

template<typename>
struct PSQLTable;

template<>
struct PSQLTable<TradingRecords> {
    static constexpr char sc_colsDefinition[] = "( real_time TIMESTAMP"
                                                ", execute_time TIMESTAMP"
                                                ", symbol VARCHAR(15)"
                                                ", price REAL"
                                                ", size INTEGER"

                                                ", trader_id_1 INTEGER"
                                                ", trader_id_2 INTEGER"
                                                ", order_id_1 VARCHAR(40)"
                                                ", order_id_2 VARCHAR(40)"
                                                ", order_type_1 VARCHAR(2)"

                                                ", order_type_2 VARCHAR(2)"
                                                ", time1 TIMESTAMP"
                                                ", time2 TIMESTAMP"
                                                ", decision CHAR"
                                                ", destination VARCHAR(10)"

                                                ",  CONSTRAINT trading_records_pkey PRIMARY KEY (order_id_1, order_id_2),\
                                                    \
                                                    CONSTRAINT trading_records_fkey_1 FOREIGN KEY (trader_id_1)\
                                                        REFERENCES public.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION,\
                                                    CONSTRAINT trading_records_fkey_2 FOREIGN KEY (trader_id_2)\
                                                        REFERENCES public.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                )";

    static constexpr char sc_recordFormat[] = "( real_time, execute_time, symbol, price, size"
                                              ", trader_id_1, trader_id_2, order_id_1, order_id_2, order_type_1"
                                              ", order_type_2, time1, time2, decision, destination"
                                              ") VALUES ";

    enum RCD_VAL_IDX : int {
        REAL_TIME = 0,
        EXEC_TIME,
        SYMBOL,
        PRICE,
        SIZE,

        TRD_ID_1,
        TRD_ID_2,
        ODR_ID_1,
        ODR_ID_2,
        ODR_TY_1,

        ODR_TY_2,
        TIME_1,
        TIME_2,
        DECISION,
        DEST,

        NUM_FIELDS
    };
};

/*static*/ constexpr char PSQLTable<TradingRecords>::sc_colsDefinition[];
/*static*/ constexpr char PSQLTable<TradingRecords>::sc_recordFormat[];

//----------------------------------------------------------------------------------------------------------------

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
    if (PQstatus(m_pConn) != CONNECTION_OK) {
        disconnectDB();
        cout << COLOR_ERROR "ERROR: Connection to database failed.\n" NO_COLOR;
        return false;
    }

    if (checkTableExist(CSTR_TBLNAME_TRADING_RECORDS) == TABLE_STATUS::NOT_EXIST) {
        // cout << CSTR_TBLNAME_TRADING_RECORDS " does not exist." << endl;
        if (createTableOfTradingRecords()) {
            cout << COLOR << '\'' << CSTR_TBLNAME_TRADING_RECORDS "' was created." NO_COLOR << endl;
        } else {
            cout << COLOR_ERROR "Error when creating " CSTR_TBLNAME_TRADING_RECORDS NO_COLOR << endl;
        }
    }
    
    return true;
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

auto DBConnector::checkTableExist(std::string tableName) -> TABLE_STATUS
{
    std::string pqQuery;
    PGresult* res = PQexec(m_pConn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cout << COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR;
        PQclear(res);
        return TABLE_STATUS::DB_ERROR;
    }
    PQclear(res);

    pqQuery = "DECLARE record CURSOR FOR SELECT * FROM pg_class WHERE relname=\'" + tableName + "\'";
    res = PQexec(m_pConn, pqQuery.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cout << COLOR_ERROR "ERROR: DECLARE CURSOR failed. (check_tbl_exist)\n" NO_COLOR;
        PQclear(res);
        return TABLE_STATUS::DB_ERROR;
    }
    PQclear(res);

    res = PQexec(m_pConn, "FETCH ALL IN record");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cout << COLOR_ERROR "ERROR: FETCH ALL failed.\n" NO_COLOR;
        PQclear(res);
        return TABLE_STATUS::DB_ERROR;
    }

    int nrows = PQntuples(res);

    PQclear(res);
    res = PQexec(m_pConn, "CLOSE record");
    PQclear(res);
    res = PQexec(m_pConn, "END");
    PQclear(res);

    if (0 == nrows) {
        return TABLE_STATUS::NOT_EXIST;
    } else if (1 == nrows) {
        return TABLE_STATUS::EXISTS;
    }
    cout << COLOR_ERROR "ERROR: More than one " << tableName << " table exist." NO_COLOR << endl;
    return TABLE_STATUS::OTHER_ERROR;
}

bool DBConnector::createTableOfTradingRecords()
{
    std::string pqQuery("CREATE TABLE " CSTR_TBLNAME_TRADING_RECORDS);
    pqQuery += PSQLTable<TradingRecords>::sc_colsDefinition;
    PGresult* res = PQexec(m_pConn, pqQuery.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cout << COLOR_ERROR "ERROR: Create table [ " CSTR_TBLNAME_TRADING_RECORDS " ] failed.\n" NO_COLOR;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
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
//     pqQuery = "INSERT INTO traders (firstname, lastname, username, password, email) VALUES ('" + client.fistName + "','" + client.lastName + "','" + client.accountName + "','" + client.password + "','" + client.email + "');";

//     PGresult* res = PQexec(m_pConn, pqQuery.c_str());

//     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
//         cout << "Error. Insert to qt_tablelist table failed.\n";
//         PQclear(res);
//     } else {
//         PQclear(res);
//         cout << "Successful Inserted!";
//     }
// }
