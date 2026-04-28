#pragma once
#include "DALExport.h"
#include "VCFData.h"
#include "IVCFDal.h"
#include "SQLiteConnectionPool.h"
#include "SQLiteQuery.h"
#include "SQLiteSelect.h"
#include "SQLiteStmt.h"
#include "VCFSQLiteRecord.h"

class DAL_API SQLiteVCFDal : public IVCFDal {
public:

    /*
    * @brief initializes the SQLiteVCFDal with a connection pool to the specified database. Optionally clears existing data.
     *
     * @param dbName The name of the SQLite database file to connect to.
     * @param clearDbTables If true, existing data in the database will be cleared during initialization. Default is false.
     * @throws std::runtime_error if the database connection fails, with the error message from SQLite.
    */
    SQLiteVCFDal(const std::string& dbName, bool clearDbTables = false);

    // Non-copyable
    SQLiteVCFDal(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal& operator=(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal(SQLiteVCFDal&& other) = delete;
    SQLiteVCFDal& operator=(SQLiteVCFDal && other) = delete;

    ~SQLiteVCFDal();

public:

    /*
    * Fetch all variants from the main table, sorted by chrom and pos
    * 
    * @throws std::runtime_error if the query fails, with the error message from SQLite.
    */
    [[nodiscard]]
    std::vector<VCFData> fetchAllVariants() override;

    /*
    * Store batch in a staging table for quick insertion
    * 
    * @param data The batch of VCFData to be stored in the staging table.
    * @throws std::runtime_error if the query fails, with the error message from SQLite.
    */
    void storeBatchInStaging(const std::vector<VCFData>& data) override;

    /*
    * Finalize records by moving them from staging to main table in sorted order
    * 
    * @throws std::runtime_error if the query fails, with the error message from SQLite.
    */
    void finalizeRecords() override;

private:
    void prepareStatments();
    void storeInVCFStaging(const std::vector<VCFSQLiteRecord>& records);

private:

    /*
    * Initialize the database
    */
    void initDb();

    /*
    * @brief delete data in the tables
    */
    void clearTables();

    /*
    * @brief Wrapper for sqlite_stmt
    * 
    * @param sql The SQL statement to prepare.
    * @param stmt Output parameter that will hold the prepared statement.
    */
    void prepare_stmt(const char* sql, sqlite3_stmt** stmt);

    /*
    * @brief Finalizes a prepared statement, releasing its resources.
    */
    void finalize_stmt(sqlite3_stmt* stmt);

private:
    class ScopedDb {
    public:
        ScopedDb(SQLiteConnectionPool& pool) : db(pool.acquire()), pool_(pool) {}
        ~ScopedDb() { pool_.release(db); }
        sqlite3* get() const { return db; }
    private:
        sqlite3* db;
        SQLiteConnectionPool& pool_;
    };
private:
    sqlite3_stmt* mInsertIntoStagingVCFRecordsStmt{ nullptr };
    sqlite3_stmt* mFetchAllVariantsStmt{ nullptr };
    SQLiteConnectionPool mConnectionPool;
};

