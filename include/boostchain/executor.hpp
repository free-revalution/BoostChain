#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

namespace boostchain {

class Executor {
public:
    explicit Executor(size_t num_threads = 0);
    ~Executor();

    // Submit task and get future
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())>;

    // Get number of worker threads
    size_t thread_count() const;

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

// Inline template implementation
template<typename F>
auto Executor::submit(F&& func) -> std::future<decltype(func())> {
    using ReturnType = decltype(func());
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(func));
    auto future = task->get_future();
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("Cannot submit task to stopped executor");
        }
        tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();
    return future;
}

} // namespace boostchain
