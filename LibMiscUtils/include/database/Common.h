#pragma once

#include "../terminal/Common.h"
#include "Tables.h"

#include <string>
#include <vector>

#include <libpq-fe.h>

namespace shift::database {

/**
     * @brief Indicates table status when querying.
     */
enum class TABLE_STATUS : int {
    NOT_EXIST = 0, // table does not exist
    EXISTS, // table exists
    DB_ERROR, // SQL database error
    OTHER_ERROR, // other error
};

auto doQuery(PGconn* const pConn, const std::string query, const std::string msgIfStatMismatch, const ExecStatusType statToMatch = ExecStatusType::PGRES_COMMAND_OK, PGresult** ppRes = nullptr) -> bool;

/**
     * @brief Check if specific table already exists
     */
auto checkTableExist(PGconn* const pConn, const std::string& tableName) -> TABLE_STATUS;

template <typename _WhichTable>
auto checkCreateTable(PGconn* const pConn) -> bool
{
    if (checkTableExist(pConn, PSQLTable<_WhichTable>::name) != TABLE_STATUS::NOT_EXIST) {
        return true; // currently just keep it like this...
    }

    // cout << PSQLTable<_WhichTable>::name << " does not exist." << endl;

    auto res = doQuery(pConn, std::string("CREATE TABLE ") + PSQLTable<_WhichTable>::name + PSQLTable<_WhichTable>::sc_colsDefinition, std::string(COLOR_ERROR "ERROR: Create table [ ") + PSQLTable<_WhichTable>::name + " ] failed.\n" NO_COLOR);

    if (res) {
        cout << COLOR << '\'' << PSQLTable<_WhichTable>::name << "' was created." NO_COLOR << endl;
    } else {
        cout << COLOR_ERROR "Error when creating " << PSQLTable<_WhichTable>::name << NO_COLOR << endl;
    }
    return res;
}

auto readRowsOfField(PGconn* const pConn, const std::string& query, int fieldIndex = 0) -> std::vector<std::string>;
auto readFieldsOfRow(PGconn* const pConn, const std::string& query, int numFields, int rowIndex = 0) -> std::vector<std::string>;

} // shift::database
