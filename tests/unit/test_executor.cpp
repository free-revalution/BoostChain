#include <boostchain/executor.hpp>
#include <cassert>
#include <iostream>
#include <chrono>
#include <atomic>

using namespace boostchain;

void test_basic_task_submission() {
    std::cout << "test_basic_task_submission: ";
    Executor executor(2);

    auto future = executor.submit([]() {
        return 42;
    });

    int result = future.get();
    assert(result == 42);
    std::cout << "PASSED\n";
}

void test_void_task() {
    std::cout << "test_void_task: ";
    Executor executor(2);

    std::atomic<int> counter{0};
    auto future = executor.submit([&counter]() {
        counter++;
    });

    future.get();
    assert(counter == 1);
    std::cout << "PASSED\n";
}

void test_multiple_tasks() {
    std::cout << "test_multiple_tasks: ";
    Executor executor(4);

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(executor.submit([i]() {
            return i * i;
        }));
    }

    for (int i = 0; i < 10; ++i) {
        int result = futures[i].get();
        assert(result == i * i);
    }
    std::cout << "PASSED\n";
}

void test_future_results() {
    std::cout << "test_future_results: ";
    Executor executor(2);

    auto future1 = executor.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return std::string("hello");
    });

    auto future2 = executor.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        return std::string("world");
    });

    // future2 should complete first since it sleeps less
    std::string result2 = future2.get();
    assert(result2 == "world");

    std::string result1 = future1.get();
    assert(result1 == "hello");
    std::cout << "PASSED\n";
}

void test_thread_count() {
    std::cout << "test_thread_count: ";
    Executor executor(3);
    assert(executor.thread_count() == 3);
    std::cout << "PASSED\n";
}

void test_default_thread_count() {
    std::cout << "test_default_thread_count: ";
    Executor executor; // Default: hardware_concurrency
    size_t count = executor.thread_count();
    assert(count > 0);
    std::cout << "PASSED (threads=" << count << ")\n";
}

void test_exception_in_task() {
    std::cout << "test_exception_in_task: ";
    Executor executor(2);

    auto future = executor.submit([]() -> int {
        throw std::runtime_error("task failed");
    });

    bool caught = false;
    try {
        future.get();
    } catch (const std::runtime_error& e) {
        caught = true;
        assert(std::string(e.what()) == "task failed");
    }
    assert(caught);
    std::cout << "PASSED\n";
}

void test_concurrent_tasks() {
    std::cout << "test_concurrent_tasks: ";
    Executor executor(4);
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(executor.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }
    assert(counter == 100);
    std::cout << "PASSED\n";
}

void test_lambda_with_capture() {
    std::cout << "test_lambda_with_capture: ";
    Executor executor(2);

    int value = 100;
    auto future = executor.submit([value]() {
        return value + 5;
    });

    assert(future.get() == 105);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Executor Tests ===\n";

    test_basic_task_submission();
    test_void_task();
    test_multiple_tasks();
    test_future_results();
    test_thread_count();
    test_default_thread_count();
    test_exception_in_task();
    test_concurrent_tasks();
    test_lambda_with_capture();

    std::cout << "\nAll Executor tests PASSED!\n";
    return 0;
}
