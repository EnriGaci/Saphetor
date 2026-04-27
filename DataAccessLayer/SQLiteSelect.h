#pragma once
#include "sqlite3.h"

#include <string>

/*
* A simple wrapper around sqlite3_stmt to facilitate SELECT queries.
* Provides methods to step through results and retrieve column values.
*/
class SQLiteSelect {
public:
    SQLiteSelect(sqlite3_stmt* stmt);

    /*
    * @brief Steps to the next row in the result set.
    */
    int step();

    /*
    * Retrieves the text value of the specified column index in the current row.
    */
    std::string column_text(int index);

    /*
    * Retrieves the integer value of the specified column index in the current row.
    */
    int column_int(int index);

    ~SQLiteSelect() = default;
private:
    sqlite3_stmt* mStmt{ nullptr };
};


