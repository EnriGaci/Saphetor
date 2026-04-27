#include "SQLiteConnectionPool.h"

#include <stdexcept>

SQLiteConnectionPool::SQLiteConnectionPool(const std::string& dbName, size_t poolSize)
    : dbName_(dbName)
{
    if (!poolSize) {
        poolSize = std::thread::hardware_concurrency();
        poolSize = poolSize ? poolSize : DEFAULT_POOL_SIZE;
    }

    for (size_t i = 0; i < poolSize; ++i) {
        sqlite3* db = nullptr;
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite database: " + dbName);
        }
        pool_.push(db);
    }
}

SQLiteConnectionPool::~SQLiteConnectionPool() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        sqlite3* db = pool_.front();
        pool_.pop();
        if (db) sqlite3_close(db);
    }
}

// Acquire a connection from the pool (blocks if none available)

sqlite3* SQLiteConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !pool_.empty(); });
    sqlite3* db = pool_.front();
    pool_.pop();
    return db;
}

// Return a connection to the pool

void SQLiteConnectionPool::release(sqlite3* db) {
    std::unique_lock<std::mutex> lock(mutex_);
    pool_.push(db);
    cond_.notify_one();
}
