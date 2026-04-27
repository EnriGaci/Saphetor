#pragma once
#include "sqlite3.h"

#include <thread>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>


#define DEFAULT_POOL_SIZE 10

class SQLiteConnectionPool {
public:
    SQLiteConnectionPool(const std::string& dbName, size_t poolSize = 0);

    ~SQLiteConnectionPool();

    /*
    * @brief Acquire a connection from the pool (blocks if none available)
    * 
    * @return A pointer to an sqlite3 connection. The caller is responsible for calling release() to return it to the pool.
    */
    sqlite3* acquire();

    /*
    * @brief Release a connection back to the pool
    */
    void release(sqlite3* db);

    // Not copyable or movable
    SQLiteConnectionPool(const SQLiteConnectionPool&) = delete;
    SQLiteConnectionPool& operator=(const SQLiteConnectionPool&) = delete;

private:
    std::string dbName_;
    std::queue<sqlite3*> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
