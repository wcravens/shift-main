#include "DBConnector.h"

#include "BCDocuments.h"

#include <shift/miscutils/crypto/Decryptor.h>
#include <shift/miscutils/terminal/Common.h>

//----------------------------------------------------------------------------------------------------------

struct TradingRecords;

template <typename>
struct PSQLTable;

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<TradingRecords> {
    static constexpr char sc_colsDefinition[] = "( real_time TIMESTAMP"
                                                ", execute_time TIMESTAMP"
                                                ", symbol VARCHAR(15)"
                                                ", price REAL"
                                                ", size INTEGER"

                                                ", trader_id_1 UUID" // UUID: Map to traders.id
                                                ", trader_id_2 UUID" // ditto
                                                ", order_id_1 VARCHAR(40)"
                                                ", order_id_2 VARCHAR(40)"
                                                ", order_type_1 VARCHAR(2)"

                                                ", order_type_2 VARCHAR(2)"
                                                ", time1 TIMESTAMP"
                                                ", time2 TIMESTAMP"
                                                ", decision CHAR"
                                                ", destination VARCHAR(10)"

                                                ",  CONSTRAINT trading_records_pkey PRIMARY KEY (order_id_1, order_id_2)\
                                                    \
                                                 ,  CONSTRAINT trading_records_fkey_1 FOREIGN KEY (trader_id_1)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                 ,  CONSTRAINT trading_records_fkey_2 FOREIGN KEY (trader_id_2)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                )";

    static constexpr char sc_recordFormat[] = "( real_time, execute_time, symbol, price, size"
                                              ", trader_id_1, trader_id_2, order_id_1, order_id_2, order_type_1"
                                              ", order_type_2, time1, time2, decision, destination"
                                              ") VALUES ";

    static const char* name;

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
/*static*/ const char* PSQLTable<TradingRecords>::name = "trading_records";

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<PortfolioSummary> {
    static constexpr char sc_colsDefinition[] = "( id UUID"
                                                ", buying_power REAL DEFAULT 1e6"
                                                ", holding_balance REAL DEFAULT 0.0"
                                                ", borrowed_balance REAL DEFAULT 0.0"
                                                ", total_pl REAL DEFAULT 0.0"

                                                ", total_shares INTEGER DEFAULT 0"

                                                ", CONSTRAINT portfolio_summary_pkey PRIMARY KEY (id)\
                                                   \
                                                 , CONSTRAINT portfolio_summary_fkey FOREIGN KEY (id)\
                                                       REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                       ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                )";

    static constexpr char sc_recordFormat[] = "( id, buying_power, holding_balance, borrowed_balance, total_pl"
                                              ", total_shares"
                                              ") VALUES ";

    static const char* name;

    enum VAL_IDX : int {
        ID,
        BP,
        HB,
        BB,
        TOTAL_PL,

        TOTAL_SH,

        NUM_FIELDS
    };
};

/*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_colsDefinition[];
/*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_recordFormat[];
/*static*/ const char* PSQLTable<PortfolioSummary>::name = "portfolio_summary";

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<PortfolioItem> {
    static constexpr char sc_colsDefinition[] = "( id UUID"
                                                ", symbol VARCHAR(15) DEFAULT '<UnknownSymbol>'"
                                                ", borrowed_balance REAL DEFAULT 0.0"
                                                ", pl REAL DEFAULT 0.0"
                                                ", long_price REAL DEFAULT 0.0"

                                                ", short_price REAL DEFAULT 0.0"
                                                ", long_shares INTEGER DEFAULT 0"
                                                ", short_shares INTEGER DEFAULT 0"

                                                ", CONSTRAINT portfolio_items_pkey PRIMARY KEY (id, symbol)\
                                                   \
                                                 , CONSTRAINT portfolio_items_fkey FOREIGN KEY (id)\
                                                       REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                       ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                )";

    static constexpr char sc_recordFormat[] = "( id, symbol, borrowed_balance, pl, long_price"
                                              ", short_price, long_shares, short_shares"
                                              ") VALUES ";

    static const char* name;

    enum VAL_IDX : int {
        ID,
        SYMBOL,
        BB,
        PL,
        LPRICE,

        SPRICE,
        LSHARES,
        SSHARES,

        NUM_FIELDS
    };
};

/*static*/ constexpr char PSQLTable<PortfolioItem>::sc_colsDefinition[];
/*static*/ constexpr char PSQLTable<PortfolioItem>::sc_recordFormat[];
/*static*/ const char* PSQLTable<PortfolioItem>::name = "portfolio_items";

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

    return checkCreateTable<TradingRecords>()
        && checkCreateTable<PortfolioSummary>()
        && checkCreateTable<PortfolioItem>();
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
            std::string userName = PQgetvalue(res1, i, 0);
            auto buyingPower = atof(PQgetvalue(res1, i, 1));
            auto shares = atoi(PQgetvalue(res2, i, 1));
            auto price = atof(PQgetvalue(res2, i, 2));

            BCDocuments::getInstance()->addRiskManagementToUserLockedExplicit(userName, buyingPower, shares, price, symbol);
            cout << userName << '\n';
        }
        cout << endl;
    }

    PQclear(res2);
    PQclear(res1);
    return res;
}

bool DBConnector::doQuery(const std::string query, const std::string msgIfStatMismatch, ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
{
    bool isMatch = true;

    PGresult* pRes = PQexec(m_pConn, query.c_str());
    if (PQresultStatus(pRes) != statToMatch) {
        cout << msgIfStatMismatch;
        isMatch = false;
    }

    if (ppRes)
        *ppRes = pRes;
    else
        PQclear(pRes);

    return isMatch;
}

auto DBConnector::checkTableExist(std::string tableName) -> TABLE_STATUS
{
    if (!doQuery("BEGIN", COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR))
        return TABLE_STATUS::DB_ERROR;

    if (!doQuery("DECLARE record CURSOR FOR SELECT * FROM pg_class WHERE relname = \'" + tableName + "\'", COLOR_ERROR "ERROR: DECLARE CURSOR failed. (check_tbl_exist)\n" NO_COLOR))
        return TABLE_STATUS::DB_ERROR;

    PGresult* pRes;
    if (!doQuery("FETCH ALL IN record", COLOR_ERROR "ERROR: FETCH ALL failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
        PQclear(pRes);
        return TABLE_STATUS::DB_ERROR;
    }

    int nrows = PQntuples(pRes);

    PQclear(pRes);
    pRes = PQexec(m_pConn, "CLOSE record");
    PQclear(pRes);
    pRes = PQexec(m_pConn, "END");
    PQclear(pRes);

    if (0 == nrows)
        return TABLE_STATUS::NOT_EXIST;
    else if (1 == nrows)
        return TABLE_STATUS::EXISTS;

    cout << COLOR_ERROR "ERROR: More than one " << tableName << " table exist." NO_COLOR << endl;
    return TABLE_STATUS::OTHER_ERROR;
}

template <typename _WhichTable>
bool DBConnector::checkCreateTable()
{
    if (checkTableExist(PSQLTable<_WhichTable>::name) != TABLE_STATUS::NOT_EXIST)
        return true; // currently just keep it like this...

    // cout << PSQLTable<_WhichTable>::name << " does not exist." << endl;

    auto res = doQuery(std::string("CREATE TABLE ") + PSQLTable<_WhichTable>::name + PSQLTable<_WhichTable>::sc_colsDefinition, std::string(COLOR_ERROR "ERROR: Create table [ ") + PSQLTable<_WhichTable>::name + " ] failed.\n" NO_COLOR);

    if (res) {
        cout << COLOR << '\'' << PSQLTable<_WhichTable>::name << "' was created." NO_COLOR << endl;
    } else {
        cout << COLOR_ERROR "Error when creating " << PSQLTable<_WhichTable>::name << NO_COLOR << endl;
    }
    return res;
}

/*static*/ std::vector<std::string> DBConnector::s_readRowsOfField(const std::string& query, int fieldIndex /*= 0*/)
{
    std::vector<std::string> vs;
    PGresult* pRes;

    if (getInstance()->doQuery(query, COLOR_ERROR "ERROR: Get rows of field[" + std::to_string(fieldIndex) + "] failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
        int rows = PQntuples(pRes);
        for (int row = 0; row < rows; row++) {
            vs.push_back(PQgetvalue(pRes, row, fieldIndex));
        }
    }

    PQclear(pRes);
    return vs;
}

/*static*/ std::vector<std::string> DBConnector::s_readFieldsOfRow(const std::string& query, int numFields, int rowIndex /*= 0*/)
{
    std::vector<std::string> vs;
    PGresult* pRes;

    if (getInstance()->doQuery(query, COLOR_ERROR "ERROR: Get fields of row[" + std::to_string(rowIndex) + "] failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)
        && 0 <= rowIndex && rowIndex < PQntuples(pRes)) {
        for (int field = 0; field < numFields; field++) // i.e. column-wise
            vs.push_back(PQgetvalue(pRes, rowIndex, field));
    }

    PQclear(pRes);
    return vs;
}
