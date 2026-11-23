#include "boofcv/concurrency.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace boofcv {
namespace {
    /// The calling thread's ThreadLimit, 0 if it has none
    thread_local uint32_t thread_limit = 0;
    /// Number of concurrent blocks the calling thread is running, so a loop_blocks() inside one is single threaded
    thread_local uint32_t inside_block = 0;
    /// See BoofConcurrency::testing_out_of_order()
    thread_local uint64_t out_of_order_loops = 0;

    /// See BoofConcurrency::testing_loops()
    thread_local uint64_t testing_loops_run = 0;

    /// The blocks of one loop_blocks() call. Each thread that works on it takes the next block until none are left
    struct Job {
        const std::function<void(uint32_t, uint32_t)>* block;
        uint64_t end;
        uint64_t size;                    // block size, or smallest block size with Guided
        uint64_t guided_divisor;          // Guided blocks are the remaining range divided by this, 0 with Fixed
        std::atomic<uint64_t> next;       // start of the next block
        uint32_t remaining;               // threads that haven't finished. Guarded by Pool::mutex
        std::exception_ptr error;         // first exception a block threw. Guarded by Pool::mutex

        /// Size of the block that starts at 'i0'
        uint64_t size_at( uint64_t i0 ) const {
            return guided_divisor == 0 ? size : std::max(size, (end - i0)/guided_divisor);
        }

        /// Runs blocks until none are left. Returns the exception a block threw, which skips the remaining blocks
        std::exception_ptr run() {
            struct Inside {
                Inside() { inside_block++; }
                ~Inside() { inside_block--; }
            } inside;
            uint64_t i0 = next.load();
            while (i0 < end) {
                const uint64_t i1 = std::min(end, i0 + size_at(i0));
                if (!next.compare_exchange_weak(i0, i1))
                    continue;   // another thread took it. i0 is now the next block's start
                try {
                    (*block)((uint32_t)i0, (uint32_t)i1);
                } catch (...) {
                    next = end;
                    return std::current_exception();
                }
                i0 = next.load();
            }
            return nullptr;
        }
    };

    /// Worker threads and the queue of jobs they join. A thread waiting for its job works on queued jobs too, so a
    /// job finishes even if the workers are busy, being replaced, or a block calls loop_blocks()
    class Pool {
    public:
        std::mutex mutex;
        std::condition_variable changed;   // a job was queued or finished, or the workers must stop
        std::deque<Job*> tasks;            // each entry asks one more thread to work on a job

        explicit Pool( uint32_t workers ) { resize(workers); }

        ~Pool() { resize(0); }

        /// Replaces the workers with 'count' new ones. Workers finish the block they're running first
        void resize( uint32_t count ) {
            std::lock_guard<std::mutex> resize_lock(resize_mutex);
            {
                std::lock_guard<std::mutex> lock(mutex);
                stop = true;
            }
            changed.notify_all();
            for (std::thread& worker : workers)
                worker.join();
            workers.clear();
            {
                std::lock_guard<std::mutex> lock(mutex);
                stop = false;
            }
            for (uint32_t i = 0; i < count; i++)
                workers.emplace_back([this] { work(); });
        }

        /// Works on the first queued job. 'lock' holds 'mutex' and is released while it works
        void run_front( std::unique_lock<std::mutex>& lock ) {
            Job* job = tasks.front();
            tasks.pop_front();
            lock.unlock();
            std::exception_ptr error = job->run();
            lock.lock();
            finished(*job, error);
        }

        /// Records that a thread finished working on 'job'. 'mutex' must be held
        void finished( Job& job, const std::exception_ptr& error ) {
            if (error && !job.error)
                job.error = error;
            if (--job.remaining == 0)
                changed.notify_all();
        }

    private:
        std::mutex resize_mutex;   // one resize at a time
        std::vector<std::thread> workers;
        bool stop = false;          // Guarded by mutex

        void work() {
            std::unique_lock<std::mutex> lock(mutex);
            while (true) {
                changed.wait(lock, [this] { return stop || !tasks.empty(); });
                if (stop)
                    return;
                run_front(lock);
            }
        }
    };

    std::atomic<uint32_t>& max_threads_value() {
        static std::atomic<uint32_t> value{BoofConcurrency::hardware_threads()};
        return value;
    }

    /// The fields of BoofConcurrency::CostModel
    struct CostModelValues {
        std::atomic<uint64_t> operations_per_us, block_min_us, schedule_operations;
    };

    CostModelValues& cost_model_values() {
        static const BoofConcurrency::CostModel defaults;
        static CostModelValues values{{defaults.operations_per_us}, {defaults.block_min_us},
                                      {defaults.schedule_operations}};
        return values;
    }

    /// Created on first use, with one worker less than max_threads(), since the calling thread works too
    Pool& pool() {
        static Pool instance(max_threads_value().load() - 1);
        return instance;
    }
}

namespace {
    std::atomic<uint32_t>& testing_blocks_value() {
        static std::atomic<uint32_t> value{0};
        return value;
    }
}

uint32_t BoofConcurrency::testing_blocks() {
    return testing_blocks_value().load();
}

void BoofConcurrency::set_testing_blocks( uint32_t blocks ) {
    testing_blocks_value() = blocks;
}

uint64_t BoofConcurrency::testing_out_of_order() {
    return out_of_order_loops;
}

uint64_t BoofConcurrency::testing_loops() {
    return testing_loops_run;
}

BoofConcurrency::CostModel BoofConcurrency::cost_model() {
    const CostModelValues& v = cost_model_values();
    CostModel model;
    model.operations_per_us = v.operations_per_us.load();
    model.block_min_us = v.block_min_us.load();
    model.schedule_operations = v.schedule_operations.load();
    return model;
}

void BoofConcurrency::set_cost_model( const CostModel& model ) {
    CostModelValues& v = cost_model_values();
    v.operations_per_us = model.operations_per_us;
    v.block_min_us = model.block_min_us;
    v.schedule_operations = model.schedule_operations;
}

uint32_t BoofConcurrency::hardware_threads() {
    return std::max(1u, std::thread::hardware_concurrency());
}

uint32_t BoofConcurrency::available_threads() {
    if (inside_block > 0)
        return 1;
    const uint32_t max = max_threads();
    return thread_limit > 0 ? std::min(max, thread_limit) : max;
}

BoofConcurrency::ThreadLimit::ThreadLimit( uint32_t threads ) : previous(thread_limit) {
    if (threads == 0)
        throw std::invalid_argument("Number of threads must be at least 1");
    thread_limit = previous > 0 ? std::min(previous, threads) : threads;
}

BoofConcurrency::ThreadLimit::~ThreadLimit() {
    thread_limit = previous;
}

uint32_t BoofConcurrency::max_threads() {
    return max_threads_value().load();
}

void BoofConcurrency::set_max_threads( uint32_t threads ) {
    if (threads == 0)
        throw std::invalid_argument("Number of threads must be at least 1");
    max_threads_value().store(threads);
    pool().resize(threads - 1);
}

namespace {
    uint32_t at_least_one( uint32_t value, const char* name ) {
        if (value == 0)
            throw std::invalid_argument(std::string(name) + " must be at least 1");
        return value;
    }
}

Schedule::Fixed& Schedule::Fixed::block_size( uint32_t size ) {
    if (sized_from_range_)
        throw std::invalid_argument("block_size can't be set with block_min or multiplier");
    block_size_ = at_least_one(size, "block_size");
    return *this;
}

Schedule::Fixed& Schedule::Fixed::block_min( uint32_t size ) {
    if (block_size_ > 0)
        throw std::invalid_argument("block_min can't be set with block_size");
    block_min_ = at_least_one(size, "block_min");
    sized_from_range_ = true;
    return *this;
}

Schedule::Fixed& Schedule::Fixed::multiplier( uint32_t blocks_per_thread ) {
    if (block_size_ > 0)
        throw std::invalid_argument("multiplier can't be set with block_size");
    multiplier_ = at_least_one(blocks_per_thread, "multiplier");
    sized_from_range_ = true;
    return *this;
}

Schedule::Guided& Schedule::Guided::block_min( uint32_t size ) {
    block_min_ = at_least_one(size, "block_min");
    return *this;
}

Schedule::Guided& Schedule::Guided::multiplier( uint32_t blocks_per_thread ) {
    multiplier_ = at_least_one(blocks_per_thread, "multiplier");
    return *this;
}

BoofConcurrency::LoopBlocks& BoofConcurrency::LoopBlocks::schedule( const Schedule::Fixed& fixed ) {
    guided_ = false;
    block_size_ = fixed.block_size_;
    block_min_ = fixed.block_min_;
    multiplier_ = fixed.multiplier_;
    return *this;
}

BoofConcurrency::LoopBlocks& BoofConcurrency::LoopBlocks::schedule( const Schedule::Guided& guided ) {
    guided_ = true;
    block_size_ = 0;
    block_min_ = guided.block_min_;
    multiplier_ = guided.multiplier_;
    return *this;
}

BoofConcurrency::LoopBlocks& BoofConcurrency::LoopBlocks::element_cost( uint64_t operations ) {
    if (operations == 0)
        throw std::invalid_argument("element_cost must be at least 1");
    element_cost_ = operations;
    return *this;
}

BoofConcurrency::LoopBlocks& BoofConcurrency::LoopBlocks::block_cost( uint64_t operations ) {
    block_cost_ = operations;
    return *this;
}

BoofConcurrency::LoopBlocks::Plan BoofConcurrency::LoopBlocks::plan() const {
    const uint64_t length = end_ > begin_ ? end_ - begin_ : 0;
    const uint32_t max = available_threads();
    const uint32_t forced = testing_blocks();
    if (block_size_ == 0 && forced > 0) {
        const uint64_t size = std::max<uint64_t>(1, (length + forced - 1)/forced);
        const auto threads = (uint32_t)std::max<uint64_t>(1, std::min<uint64_t>(max, (length + size - 1)/size));
        return {size, threads, false, true};
    }
    uint64_t size = block_size_;
    if (size == 0) {
        size = guided_ ? block_min_ : std::max<uint64_t>(block_min_, length/((uint64_t)max*multiplier_));
        if (element_cost_ > 0) {
            const CostModel model = cost_model();
            auto elements = [&]( uint64_t operations ) { return (operations + element_cost_ - 1)/element_cost_; };
            size = std::max({size, elements(model.block_min_us*model.operations_per_us),
                             elements(10*(block_cost_ + model.schedule_operations))});
        }
    }
    // A thread for each full block, so a part of a block isn't worth waking a thread
    const auto threads = (uint32_t)std::max<uint64_t>(1, std::min<uint64_t>(max, length/size));
    return {size, threads, guided_, false};
}

BoofConcurrency::LoopBlocks& BoofConcurrency::LoopBlocks::single( std::function<void()> single ) {
    single_ = std::move(single);
    return *this;
}

void BoofConcurrency::LoopBlocks::run( const std::function<void(uint32_t, uint32_t)>& block ) const {
    if (end_ <= begin_)
        return;
    const Plan plan = this->plan();
    if (plan.testing)
        testing_loops_run++;
    const uint64_t size = plan.size;
    const uint32_t threads = plan.threads;
    if (threads == 1) {
        if (single_) {
            single_();
        } else if (plan.guided) {
            block(begin_, end_);
        } else {
            for (uint64_t i0 = begin_; i0 < end_; i0 += size)
                block((uint32_t)i0, (uint32_t)std::min<uint64_t>(end_, i0 + size));
        }
        return;
    }

    // With testing blocks, which are all 'size', the n-th block to finish must be block n for an in-order loop
    std::atomic<uint64_t> finished{0};
    std::atomic<bool> out_of_order{false};
    std::function<void(uint32_t, uint32_t)> recorded;
    if (plan.testing) {
        recorded = [&]( uint32_t i0, uint32_t i1 ) {
            block(i0, i1);
            if (finished++ != (i0 - begin_)/size)
                out_of_order = true;
        };
    }

    Pool& p = pool();
    Job job;
    job.block = plan.testing ? &recorded : &block;
    job.end = end_;
    job.size = size;
    job.guided_divisor = plan.guided ? (uint64_t)threads*multiplier_ : 0;
    job.next = begin_;
    job.remaining = threads;
    {
        std::lock_guard<std::mutex> lock(p.mutex);
        for (uint32_t i = 1; i < threads; i++)
            p.tasks.push_back(&job);
    }
    p.changed.notify_all();

    std::exception_ptr error = job.run();

    // Help with queued jobs, so this one finishes even if every worker is busy
    std::unique_lock<std::mutex> lock(p.mutex);
    p.finished(job, error);
    while (job.remaining > 0) {
        if (!p.tasks.empty())
            p.run_front(lock);
        else
            p.changed.wait(lock);
    }
    if (out_of_order)
        out_of_order_loops++;
    if (job.error)
        std::rethrow_exception(job.error);
}
}
