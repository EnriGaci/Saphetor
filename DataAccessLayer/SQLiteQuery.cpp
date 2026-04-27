#include "SQLiteQuery.h"

#include <stdexcept>

SQLiteQuery::SQLiteQuery(SQLiteConnectionPool& pool) : mConnectionPool(pool) {
    mDb = pool.acquire();
}

/**
* Executes a non-SELECT SQL statement (e.g., INSERT, UPDATE, DELETE).
*
* @throws std::runtine error if the execution fails, with the error message from SQLite.
*/

void SQLiteQuery::exec(const std::string& sql) {
    char* errMsg = nullptr;
    auto status = sqlite3_exec(mDb, sql.c_str(), nullptr, nullptr, &errMsg);
    if (status != SQLITE_OK) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

SQLiteQuery::~SQLiteQuery()
{
    if (mDb) {
        mConnectionPool.release(mDb);
    }
}
