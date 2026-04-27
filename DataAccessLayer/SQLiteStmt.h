#pragma once
#include "SQLiteQuery.h"

/*
* @brief A wrapper around sqlite3_stmt to facilitate prepared statements with parameter binding.
*/
class SQLiteStmt : public SQLiteQuery {
public:

    SQLiteStmt(SQLiteConnectionPool& pool, sqlite3_stmt* stmt);

    /*
    * @brief Binds a text value to the specified parameter index (1-based).
    * 
    * @param pos The 1-based index of the parameter to bind.
    * @param value The text value to bind.
    */
    void bind_text(int pos, const char* value);

    /*
    * @brief Binds an integer value to the specified parameter index (1-based).
    * 
    * * @param pos The 1-based index of the parameter to bind.
    * @param value The integer value to bind.
    */
    void bind_int(int pos, int value);

    /*
    * @brief Finalizes the bindings and executes the statement. This should be called after all parameters have been bound.
    * It will execute the statement and reset it for the next execution, allowing for multiple sets of parameters to be bound and executed in a loop.
    */
    void finalizeBindings();

    ~SQLiteStmt();
private:
    sqlite3_stmt* mStmt;
};


