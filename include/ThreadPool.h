#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

using RWMutex = std::mutex;
using WriteLock = std::unique_lock<RWMutex>;


class ThreadPool {
public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { this->worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<RWMutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all(); // Wake everyone up to exit
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

    void waitForTasksToFinish() {
        std::unique_lock<RWMutex> lock(queue_mutex);
        tasks_done_condition.wait(lock, [this] { return tasks.empty() && active_tasks == 0; });
    }

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

    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                WriteLock lock(queue_mutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });
                if (stop && tasks.empty()) {
                    return; // Exit thread
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            {
                WriteLock lock(queue_mutex);
                active_tasks++;
            }

            task(); // Execute the task

            {
                WriteLock lock(queue_mutex);
                active_tasks--;
                if (tasks.empty() && active_tasks == 0) {
                    tasks_done_condition.notify_all(); // Notify that all tasks are done
                }
            }
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    RWMutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable tasks_done_condition;
    size_t active_tasks = 0;
    bool stop = false;
};
