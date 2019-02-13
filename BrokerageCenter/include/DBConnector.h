/*
** DBConnector maintains the connection between BC and database
*/

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <postgresql/libpq-fe.h>

struct UserInfo {
    std::string id;
    std::string fistName;
    std::string lastName;
    std::string accountName;
    std::string password;
    double buyingPower;
    std::string email;
    char gender;
};

class DBConnector {
public:
    ~DBConnector();

    std::unique_lock<std::mutex> lockPSQL() const;

    static DBConnector* getInstance();
    bool init(const std::string& cryptoKey, const std::string& fileName);

    PGconn* getConn() { return m_pConn; } // establish connection to database
    bool connectDB();
    void disconnectDB();

    bool createUsers(const std::string& symbol);

    /*@brief Indicates table status when querying */
    enum class TABLE_STATUS : int {
        NOT_EXIST = 0, // table does not exist
        EXISTS, // table exists
        DB_ERROR, // SQL database error
        OTHER_ERROR, // other error
    };

    bool doQuery(const std::string query, const std::string msgIfStatMismatch, ExecStatusType statToMatch = PGRES_COMMAND_OK, PGresult** ppRes = nullptr);

    /*@brief Check if specific table already exists */
    TABLE_STATUS checkTableExist(std::string tableName);

    /*@brief Check if specific table already exists; if not, create it */
    template <typename _WhichTable>
    bool checkCreateTable();

    static std::vector<std::string> s_readRowsOfField(const std::string& query, int fieldIndex = 0);
    static std::vector<std::string> s_readFieldsOfRow(const std::string& query, int numFields, int rowIndex = 0);

private:
    DBConnector(); /* singleton pattern */
    std::unordered_map<std::string, std::string> m_loginInfo;

protected:
    PGconn* m_pConn;
    mutable std::mutex m_mtxPSQL;
};
