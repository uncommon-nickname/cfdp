#include <cfdp_runtime/logger.hpp>
#include <cfdp_runtime/thread_pool.hpp>

cfdp::runtime::thread_pool::ThreadPool::ThreadPool(size_t numWorkers) : shutdownFlag(false), queue()
{
    logging::trace("creating a thread pool object with {} worker(s)", numWorkers);

    workers = std::vector<std::thread>{};
    workers.reserve(numWorkers);

    for (size_t i = 0; i < numWorkers; ++i)
    {
        // We can safely pass a `this` reference to every worker.
        // ThreadPool object should always outlive its children.
        auto worker = std::thread{[this]() mutable {
            while (!shutdownFlag.load(std::memory_order_relaxed))
            {
                auto potentialTask = queue.tryPop();

                if (potentialTask.has_value())
                {
                    auto task = potentialTask.value();

                    logging::trace("worker picked up a task, queue size: {}", queue.sizeNow());

                    task();
                }
            }
        }};

        workers.emplace_back(std::move(worker));
    }
}

void cfdp::runtime::thread_pool::ThreadPool::shutdown() noexcept
{
    if (shutdownFlag.exchange(true, std::memory_order_relaxed))
    {
        logging::warn("thread pool object was already closed, skipping shutdown");
        return;
    }

    logging::trace("shutting down a thread pool object");

    for (auto& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}
