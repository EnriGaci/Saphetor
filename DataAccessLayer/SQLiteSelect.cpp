#include "SQLiteSelect.h"

SQLiteSelect::SQLiteSelect(sqlite3_stmt* stmt) : mStmt(stmt) { }

int SQLiteSelect::step() {
    return sqlite3_step(mStmt);
}

std::string SQLiteSelect::column_text(int index) {
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(mStmt, index));
    return text ? text : "";
}

int SQLiteSelect::column_int(int index) {
    return sqlite3_column_int(mStmt, index);
}
