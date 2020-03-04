#include "database/Common.h"

#include "terminal/Common.h"

#include <string>

namespace shift {
namespace database {

    /**
     * @brief: Used to issue queries. IE: CREATE, BEGIN ,SELECT etc.
     * @param pConn: psql connection
     * @param query: string query
     * @param msgIfStatMismatch: string, to print if the query failed
     * @param statToMatch: psql status condition expected.
     *        @NOTE: If you want to execute a result that returns tuples, use the flag PGRES_TUPLES_OK
     * @param ppRes: double pointer to the target PGresult object. call like (&pRes)
     */
    bool doQuery(PGconn* const pConn, const std::string query, const std::string msgIfStatMismatch, const ExecStatusType statToMatch /*= PGRES_COMMAND_OK*/, PGresult** ppRes /*= nullptr*/)
    {
        bool isMatch = true;

        PGresult* pRes = PQexec(pConn, query.c_str());
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

    TABLE_STATUS checkTableExist(PGconn* const pConn, const std::string& tableName)
    {
        if (!doQuery(pConn, "BEGIN", COLOR_ERROR "ERROR: BEGIN command failed.\n" NO_COLOR))
            return TABLE_STATUS::DB_ERROR;

        if (!doQuery(pConn, "DECLARE record CURSOR FOR SELECT * FROM pg_class WHERE relname = \'" + tableName + "\'", COLOR_ERROR "ERROR: DECLARE CURSOR failed. (check_tbl_exist)\n" NO_COLOR))
            return TABLE_STATUS::DB_ERROR;

        PGresult* pRes;
        if (!doQuery(pConn, "FETCH ALL IN record", COLOR_ERROR "ERROR: FETCH ALL failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
            PQclear(pRes);
            return TABLE_STATUS::DB_ERROR;
        }

        int nrows = PQntuples(pRes);

        PQclear(pRes);
        pRes = PQexec(pConn, "CLOSE record");
        PQclear(pRes);
        pRes = PQexec(pConn, "END");
        PQclear(pRes);

        if (0 == nrows)
            return TABLE_STATUS::NOT_EXIST;
        else if (1 == nrows)
            return TABLE_STATUS::EXISTS;

        cout << COLOR_ERROR "ERROR: More than one " << tableName << " table exist." NO_COLOR << endl;
        return TABLE_STATUS::OTHER_ERROR;
    }

    /**
     * Used to retrieve all field entries in every row.
     * EX:
     *  readRowsOfField(...,...,1) of the following table:
     * |row1|row2|row3|
     *  A    B    C
     *  D    E    F
     *  G    H    I
     * 
     * Will return: [B,E,H]
     */
    std::vector<std::string> readRowsOfField(PGconn* const pConn, const std::string& query, int fieldIndex /*= 0*/)
    {
        std::vector<std::string> vs;
        PGresult* pRes;

        if (doQuery(pConn, query, COLOR_ERROR "ERROR: Get rows of field[" + std::to_string(fieldIndex) + "] failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)) {
            int rows = PQntuples(pRes);
            for (int row = 0; row < rows; ++row) {
                vs.push_back(PQgetvalue(pRes, row, fieldIndex));
            }
        }

        PQclear(pRes);
        return vs;
    }

    /**
     * Used to retrieve all entries of a single row.
     * EX:
     *  readRowsOfField(...,...,2,1) of the following table:
     * |row1|row2|row3|
     *  A    B    C
     *  D    E    F
     *  G    H    I
     * 
     * Will return: [G,H]
     */
    std::vector<std::string> readFieldsOfRow(PGconn* const pConn, const std::string& query, int numFields, int rowIndex /*= 0*/)
    {
        std::vector<std::string> vs;
        PGresult* pRes;

        if (doQuery(pConn, query, COLOR_ERROR "ERROR: Get fields of row[" + std::to_string(rowIndex) + "] failed.\n" NO_COLOR, PGRES_TUPLES_OK, &pRes)
            && 0 <= rowIndex && rowIndex < PQntuples(pRes)) {
            for (int field = 0; field < numFields; ++field) // i.e. column-wise
                vs.push_back(PQgetvalue(pRes, rowIndex, field));
        }

        PQclear(pRes);
        return vs;
    }

} // database
} // shift