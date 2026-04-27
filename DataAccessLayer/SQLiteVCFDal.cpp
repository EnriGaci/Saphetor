#include "SQLiteVCFDal.h"

SQLiteVCFDal::SQLiteVCFDal(const std::string& dbName) : mConnectionPool(dbName, 10) {
    prepareStatments();
}

SQLiteVCFDal::~SQLiteVCFDal() {
    if (mFetchAllVariantsStmt) finalize_stmt(mFetchAllVariantsStmt);
    if (mInsertIntoStagingVCFRecordsStmt) finalize_stmt(mInsertIntoStagingVCFRecordsStmt);
}

void SQLiteVCFDal::initDb(bool clearDb) {
    // Implement database initialization logic here, e.g., create tables if they don't exist
    SQLiteQuery query(mConnectionPool);

    // Performance Tuning
    query.exec("PRAGMA journal_mode = WAL;"); // Better concurrency
    query.exec("PRAGMA synchronous = NORMAL;"); // Faster writes

    if (clearDb) {
        const std::string droptables = R"(
                DROP TABLE IF EXISTS variants;
                DROP TABLE IF EXISTS staging_variants;
            )";
        query.exec(droptables);
    }

    const std::string createVariantsTable = R"(
            CREATE TABLE IF NOT EXISTS variants (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chromosome TEXT,
                position INTEGER,
                ref TEXT,
                alt TEXT,
                data TEXT
            );

            -- Nnon-unique index to keep sorting fast
            CREATE INDEX IF NOT EXISTS idx_coords ON variants (chromosome, position);

        )";
    query.exec(createVariantsTable);

    const std::string constraints = R"(
            ALTER TABLE variants 
            ADD CONSTRAINT valid_json 
            CHECK (json_valid(data));
        )";

    try {
        query.exec(constraints);
    }
    catch (const std::exception& ex) {
        // Ignore error if constraint already exists
        if (std::string(ex.what()).find("already exists") == std::string::npos) {
            throw; // Only re-throw if it's NOT the "already exists" error
        }
    }

    const std::string createVCFTempTable = R"(
            CREATE TABLE IF NOT EXISTS staging_variants (
                chromosome TEXT,
                position INTEGER,
                ref TEXT,
                alt TEXT,
                data TEXT
            );
        )";
    query.exec(createVCFTempTable);

}

void SQLiteVCFDal::prepareStatments() {
    // prepare statements 
    const char* sqlInsertStagingVCFRecord = "INSERT INTO staging_variants VALUES (?, ?, ?, ?, ?);";
    prepare_stmt(sqlInsertStagingVCFRecord, &mInsertIntoStagingVCFRecordsStmt);

    const char* sqlFetchAllVariantsStmt = R"(
            SELECT CHROMOSOME, POSITION, REF, ALT, DATA
            FROM variants
            ORDER BY CHROMOSOME, POSITION; 
        )";

    prepare_stmt(sqlFetchAllVariantsStmt, &mFetchAllVariantsStmt);
}

[[nodiscard]]
std::vector<VCFData> SQLiteVCFDal::fetchAllVariants() {
    std::vector<VCFData> result;

    SQLiteSelect select(mFetchAllVariantsStmt);

    while (select.step() == SQLITE_ROW) {
        VCFSQLiteRecord record;

        record.chromosome = select.column_text(0);
        record.position = select.column_int(1);
        record.ref = select.column_text(2);
        record.alt = select.column_text(3);
        record.data = select.column_text(4);

        result.emplace_back(record.toVCFData());
    }

    return result;
}

void SQLiteVCFDal::storeBatchInStaging(const std::vector<VCFData>& data) {
    // Implement batch storage logic here
    std::vector<VCFSQLiteRecord> records;
    records.reserve(data.size());

    for (const auto& item : data) {
        VCFSQLiteRecord record(item);
        records.push_back(std::move(record));
    }

    storeInVCFStaging(records);
}

void SQLiteVCFDal::finalizeRecords() {

    SQLiteQuery query(mConnectionPool);

    const char* sql = R"(
        INSERT INTO variants (chromosome, position, ref, alt, data)
            SELECT CHROMOSOME, POSITION, REF, ALT, DATA
            FROM staging_variants
            ORDER BY CHROMOSOME, POSITION ;

        DELETE FROM staging_variants;
        )";

    query.exec(sql);
}

void SQLiteVCFDal::storeInVCFStaging(const std::vector<VCFSQLiteRecord>& records) {
    SQLiteStmt stmt(mConnectionPool, mInsertIntoStagingVCFRecordsStmt);

    for (auto record : records) {
        stmt.bind_text(1, record.chromosome.c_str());
        stmt.bind_int(2, record.position);
        stmt.bind_text(3, record.ref.c_str());
        stmt.bind_text(4, record.alt.c_str());
        stmt.bind_text(5, record.data.c_str());

        stmt.finalizeBindings();
    }
}

void SQLiteVCFDal::prepare_stmt(const char* sql, sqlite3_stmt** stmt) {
    ScopedDb db(mConnectionPool);

    if (sqlite3_prepare_v2(db.get(), sql, -1, stmt, NULL) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db.get()));
    }
}

void SQLiteVCFDal::finalize_stmt(sqlite3_stmt* stmt) {
    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        throw std::runtime_error("Error when finalizing statement");
    }
}

