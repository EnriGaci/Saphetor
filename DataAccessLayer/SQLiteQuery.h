#pragma once
#include "DALExport.h"
#include "SQLiteConnectionPool.h"

#include "sqlite3.h"

/*
* Wrapper for queries
* Uses RAII to manage db connection
*/
class DAL_API SQLiteQuery {
public:
    SQLiteQuery(SQLiteConnectionPool& pool);

    /**
    * Executes a non-SELECT SQL statement (e.g., INSERT, UPDATE, DELETE).
    * 
    * @throws std::runtine error if the execution fails, with the error message from SQLite.
    */
    void exec(const std::string& sql);

    virtual ~SQLiteQuery();

protected:
    sqlite3* mDb{ nullptr };
    SQLiteConnectionPool& mConnectionPool;
};


