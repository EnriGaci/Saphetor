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
    SQLiteVCFDal(const std::string& dbName);

    // Non-copyable
    SQLiteVCFDal(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal& operator=(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal(SQLiteVCFDal&& other) = delete;
    SQLiteVCFDal& operator=(SQLiteVCFDal && other) = delete;

    ~SQLiteVCFDal();

public:

    /*
    * Initialize the database
    * 
    * @param clearDb If true, existing data will be cleared. Default is false.
    */
    void initDb(bool clearDb = false) override;


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
    void prepare_stmt(const char* sql, sqlite3_stmt** stmt);
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

