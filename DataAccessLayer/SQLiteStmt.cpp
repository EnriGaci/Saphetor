#include "SQLiteStmt.h"

#include <stdexcept>

SQLiteStmt::SQLiteStmt(SQLiteConnectionPool& pool, sqlite3_stmt* stmt) : SQLiteQuery(pool) {

    if (!stmt) {
        throw std::runtime_error("Invalid statement");
    }

    exec("BEGIN TRANSACTION");
    mStmt = stmt;
}

void SQLiteStmt::bind_text(int pos, const char* value) {
    sqlite3_bind_text(mStmt, pos, value, -1, SQLITE_STATIC);
}

void SQLiteStmt::bind_int(int pos, int value) {
    sqlite3_bind_int(mStmt, pos, value);
}

void SQLiteStmt::finalizeBindings() {
    sqlite3_step(mStmt);   // Execute
    sqlite3_reset(mStmt);  // Reset for the next set of values
    sqlite3_clear_bindings(mStmt);
}

SQLiteStmt::~SQLiteStmt() {
    // 4. Commit the Transaction flushing all rows to disk at once
    exec("COMMIT;");
}
