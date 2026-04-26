#pragma once
#include "CommonExport.h"

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>


using RWMutex = std::mutex;
using WriteLock = std::unique_lock<RWMutex>;



class COMMON_API ThreadPool {
public:
    ThreadPool(size_t num_threads);

    ~ThreadPool();

    void waitForTasksToFinish();

    // Template to accept any function/lambda
    template<class F>
    void enqueue(F&& f) {
        {
            std::lock_guard<RWMutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one(); // Wake up one worker
    }

private:
    void worker_loop();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    RWMutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable tasks_done_condition;
    size_t active_tasks = 0;
    bool stop = false;
};

