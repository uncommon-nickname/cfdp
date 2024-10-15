#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "atomic_queue.hpp"
#include "future.hpp"
#include "logger.hpp"

namespace cfdp::runtime::thread_pool
{
class ThreadPool
{
  public:
    explicit ThreadPool(size_t numWorkers = std::thread::hardware_concurrency() * 2 + 1);
    ~ThreadPool() { shutdown(); };

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(ThreadPool const&) = delete;
    ThreadPool(ThreadPool&&)                 = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    template <class Functor>
        requires std::invocable<Functor>
    auto dispatchTask(Functor&& func) noexcept -> future::Future<decltype(func())>;

    void shutdown() noexcept;

  private:
    std::atomic_bool shutdownFlag;
    std::vector<std::thread> workers;
    atomic::AtomicQueue<std::function<void()>> queue;
};
} // namespace cfdp::runtime::thread_pool

namespace
{
using ::cfdp::runtime::atomic::AtomicQueue;
using ::cfdp::runtime::future::Future;
using ::cfdp::runtime::thread_pool::ThreadPool;
} // namespace

template <class Functor>
    requires std::invocable<Functor>
auto ThreadPool::dispatchTask(Functor&& func) noexcept -> Future<decltype(func())>
{
    // Unfortunately std::function does not provide a constructor for
    // moving lambdas. We can't directly move the promise to the task,
    // so as a W/A we can wrap it into std::shared_ptr and copy it.
    auto promise = std::make_shared<std::promise<decltype(func())>>();

    logging::trace("dispatching a new task, queue size: {}", queue.sizeNow());

    std::function<void()> wrapper = [promise, func]() mutable {
        try
        {
            auto result = std::invoke(func);
            promise->set_value(result);
        }
        catch (...)
        {
            promise->set_exception(std::current_exception());
        }
    };

    queue.emplace(std::move(wrapper));

    return Future{promise->get_future()};
}
