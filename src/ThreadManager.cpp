#include "../include/ThreadManager.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

ThreadManager::ThreadManager(size_t numThreads)
        : numThreads(numThreads), running(false), activeThreads(0) {}

ThreadManager::~ThreadManager() {
    stop();
}

void ThreadManager::start() {
    running = true;
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this]() {
            while (running) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait(lock, [this]() {
                        return !taskQueue.empty() || !running;
                    });

                    if (!running && taskQueue.empty()) return;

                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                    ++activeThreads;
                }

                task();

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    --activeThreads;
                    if (taskQueue.empty() && activeThreads == 0) {
                        completionCondition.notify_all();
                    }
                }
            }
        });
    }
}

void ThreadManager::stop() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }

    queueCondition.notify_all();

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    threads.clear();
}

void ThreadManager::addTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(std::move(task));
    }
    queueCondition.notify_one();
}

bool ThreadManager::isRunning() const {
    return running;
}

void ThreadManager::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queueMutex);
    completionCondition.wait(lock, [this]() {
        return taskQueue.empty() && activeThreads == 0;
    });
}

size_t ThreadManager::getTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return taskQueue.size();
}

size_t ThreadManager::getActiveThreadCount() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return activeThreads;
}

void ThreadManager::setNumThreads(size_t newNumThreads) {
    stop();
    numThreads = newNumThreads;
    start();
}

void ThreadManager::processNextTask() {
    // Currently empty - should be implemented or removed
}

// Missing error handling in thread loop
while (running) {
try {
// Existing task processing code
} catch (...) {
// Handle exceptions to prevent thread crashes
}
}