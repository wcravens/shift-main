#pragma once

#include "Tables.h"

#include <shift/miscutils/terminal/Common.h>

#include <postgresql/libpq-fe.h>

#include <string>
#include <vector>

namespace shift {
namespace database {

    /*@brief Indicates table status when querying */
    enum class TABLE_STATUS : int {
        NOT_EXIST = 0, // table does not exist
        EXISTS, // table exists
        DB_ERROR, // SQL database error
        OTHER_ERROR, // other error
    };

    /*@brief Check if specific table already exists */
    TABLE_STATUS checkTableExist(PGconn* const pConn, const std::string& tableName);

    bool doQuery(PGconn* const pConn, const std::string query, const std::string msgIfStatMismatch, const ExecStatusType statToMatch = ExecStatusType::PGRES_COMMAND_OK, PGresult** ppRes = nullptr);

    template <typename _WhichTable>
    bool checkCreateTable(PGconn* const pConn)
    {
        if (checkTableExist(pConn, PSQLTable<_WhichTable>::name) != TABLE_STATUS::NOT_EXIST)
            return true; // currently just keep it like this...

        // cout << PSQLTable<_WhichTable>::name << " does not exist." << endl;

        auto res = doQuery(pConn, std::string("CREATE TABLE ") + PSQLTable<_WhichTable>::name + PSQLTable<_WhichTable>::sc_colsDefinition, std::string(COLOR_ERROR "ERROR: Create table [ ") + PSQLTable<_WhichTable>::name + " ] failed.\n" NO_COLOR);

        if (res) {
            cout << COLOR << '\'' << PSQLTable<_WhichTable>::name << "' was created." NO_COLOR << endl;
        } else {
            cout << COLOR_ERROR "Error when creating " << PSQLTable<_WhichTable>::name << NO_COLOR << endl;
        }
        return res;
    }

    std::vector<std::string> readRowsOfField(PGconn* const pConn, const std::string& query, int fieldIndex = 0);
    std::vector<std::string> readFieldsOfRow(PGconn* const pConn, const std::string& query, int numFields, int rowIndex = 0);

} // database
} // shift