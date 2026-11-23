#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/concurrency.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

using namespace std;
using namespace boofcv;

/// Restores the number of threads after each test
class BoofConcurrencyTest : public boofcv::BoofCPPTest {
protected:
    const uint32_t original = BoofConcurrency::max_threads();

    void TearDown() override {
        BoofConcurrency::set_max_threads(original);
        BoofConcurrency::set_cost_model({});
        BoofConcurrency::set_testing_blocks(0);
    }

    /// Blocks that 'loop' passed, in increasing order
    static vector<pair<uint32_t, uint32_t>> blocks_of( const BoofConcurrency::LoopBlocks& loop ) {
        mutex m;
        vector<pair<uint32_t, uint32_t>> blocks;
        loop.run([&]( uint32_t i0, uint32_t i1 ) {
            lock_guard<mutex> lock(m);
            blocks.emplace_back(i0, i1);
        });
        sort(blocks.begin(), blocks.end());
        return blocks;
    }

    static vector<pair<uint32_t, uint32_t>> fixed_blocks( uint32_t begin, uint32_t end, uint32_t block_size ) {
        return blocks_of(BoofConcurrency::loop_blocks(begin, end).schedule(Schedule::Fixed().block_size(block_size)));
    }

    /// True if run() calls the single threaded function instead of the blocks
    static bool runs_single( BoofConcurrency::LoopBlocks loop ) {
        bool called = false;
        loop.single([&] { called = true; }).run([]( uint32_t, uint32_t ) {});
        return called;
    }

    /// The blocks are contiguous and cover [begin, end) once
    static void check_cover( const vector<pair<uint32_t, uint32_t>>& blocks, uint32_t begin, uint32_t end ) {
        uint32_t expected = begin;
        for (const auto& block : blocks) {
            EXPECT_EQ(expected, block.first);
            EXPECT_LT(block.first, block.second);
            expected = block.second;
        }
        EXPECT_EQ(end, expected);
    }

    const vector<pair<uint32_t, uint32_t>> ranges = {{0, 1}, {3, 4}, {0, 7}, {5, 13}, {10, 1000}, {7, 8000}};
};

// Defined first, so no test has changed the number of threads yet
TEST_F(BoofConcurrencyTest, default_is_hardware) {
    EXPECT_GE(BoofConcurrency::hardware_threads(), 1u);
    EXPECT_EQ(BoofConcurrency::hardware_threads(), BoofConcurrency::max_threads());
}

TEST_F(BoofConcurrencyTest, set_max_threads) {
    BoofConcurrency::set_max_threads(3);
    EXPECT_EQ(3u, BoofConcurrency::max_threads());
    BoofConcurrency::set_max_threads(1);
    EXPECT_EQ(1u, BoofConcurrency::max_threads());
    EXPECT_THROW(BoofConcurrency::set_max_threads(0), invalid_argument);
    EXPECT_EQ(1u, BoofConcurrency::max_threads());
}

/// Fixed blocks of 'block_size' cover the range once, the last one possibly smaller, for any number of threads
TEST_F(BoofConcurrencyTest, fixed_split) {
    for (uint32_t threads : {1u, 3u, 9u}) {
        BoofConcurrency::set_max_threads(threads);
        for (const auto& range : ranges) {
            for (uint32_t block_size : {1u, 3u, 64u, 10000u}) {
                auto blocks = fixed_blocks(range.first, range.second, block_size);
                const uint32_t length = range.second - range.first;
                ASSERT_EQ((length + block_size - 1)/block_size, blocks.size());
                for (size_t i = 0; i < blocks.size(); i++) {
                    EXPECT_EQ(range.first + i*block_size, blocks[i].first);
                    EXPECT_EQ(min(range.second, blocks[i].first + block_size), blocks[i].second);
                }
            }
        }
    }
}

/// Without block_size(), Fixed uses max(block_min, length/(threads*multiplier))
TEST_F(BoofConcurrencyTest, default_block_size_fixed) {
    BoofConcurrency::set_max_threads(4);
    auto size_of = []( uint32_t end, const Schedule::Fixed& fixed ) {
        auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, end).schedule(fixed));
        return blocks.at(0).second - blocks.at(0).first;
    };
    EXPECT_EQ(62u, size_of(1000, Schedule::Fixed()));
    EXPECT_EQ(80u, size_of(1000, Schedule::Fixed().block_min(80)));
    EXPECT_EQ(125u, size_of(1000, Schedule::Fixed().multiplier(2)));
    EXPECT_EQ(7u, size_of(1000, Schedule::Fixed().block_size(7)));
    // Fewer elements than threads*multiplier
    EXPECT_EQ(1u, size_of(10, Schedule::Fixed()));
}

/// Guided's smallest block is block_min
TEST_F(BoofConcurrencyTest, guided_block_min) {
    BoofConcurrency::set_max_threads(4);
    for (uint32_t block_min : {1u, 3u, 20u}) {
        auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 1000).schedule(Schedule::Guided().block_min(block_min)));
        check_cover(blocks, 0, 1000);
        EXPECT_EQ(make_pair(0u, 62u), blocks.at(0));
        // Before the last block, which can be smaller, is one of exactly block_min
        const auto& block = blocks.at(blocks.size() - 2);
        EXPECT_EQ(block_min, block.second - block.first);
    }
}

/// Guided blocks cover the range once and get smaller towards the end, but only the last one is smaller than
/// block_min
TEST_F(BoofConcurrencyTest, guided_split) {
    for (uint32_t threads : {1u, 3u, 9u}) {
        BoofConcurrency::set_max_threads(threads);
        for (const auto& range : ranges) {
            for (uint32_t block_min : {1u, 3u, 64u}) {
                auto blocks = blocks_of(BoofConcurrency::loop_blocks(range.first, range.second)
                        .schedule(Schedule::Guided().block_min(block_min)));
                check_cover(blocks, range.first, range.second);
                for (size_t i = 0; i + 1 < blocks.size(); i++) {
                    const uint32_t size = blocks[i].second - blocks[i].first;
                    EXPECT_GE(size, block_min);
                    if (i + 2 < blocks.size())
                        EXPECT_GE(size, blocks[i + 1].second - blocks[i + 1].first);
                }
                if (threads == 1)
                    EXPECT_EQ(1u, blocks.size());
            }
        }
    }
}

/// A Guided block is the remaining range divided by the number of threads times multiplier()
TEST_F(BoofConcurrencyTest, guided_sizes) {
    BoofConcurrency::set_max_threads(4);
    auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 1000).schedule(Schedule::Guided()));
    EXPECT_EQ(make_pair(0u, 62u), blocks.at(0));      // 1000/16
    EXPECT_EQ(make_pair(62u, 120u), blocks.at(1));    // 62 + 938/16
    blocks = blocks_of(BoofConcurrency::loop_blocks(0, 1000).schedule(Schedule::Guided().multiplier(2)));
    EXPECT_EQ(make_pair(0u, 125u), blocks.at(0));     // 1000/8
    EXPECT_EQ(make_pair(125u, 234u), blocks.at(1));   // 125 + 875/8
}

/// With element_cost(), a block is at least block_min_us of work and its overhead at most a tenth of it
TEST_F(BoofConcurrencyTest, element_cost) {
    const BoofConcurrency::CostModel model;
    BoofConcurrency::set_max_threads(4);
    const uint32_t min_time = (uint32_t)(model.block_min_us*model.operations_per_us/1000);   // elements of 1000 ops
    auto smallest = []( const vector<pair<uint32_t, uint32_t>>& blocks ) {
        uint32_t size = blocks.at(0).second - blocks.at(0).first;
        for (size_t i = 0; i + 1 < blocks.size(); i++)
            size = min(size, blocks[i].second - blocks[i].first);
        return size;
    };
    auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 10000).element_cost(1000));
    check_cover(blocks, 0, 10000);
    EXPECT_EQ(min_time, smallest(blocks));

    // block_cost() makes the blocks larger, so the overhead is at most a tenth
    const uint32_t min_overhead = (uint32_t)(10*(200000 + model.schedule_operations)/1000);
    ASSERT_GT(min_overhead, min_time);
    blocks = blocks_of(BoofConcurrency::loop_blocks(0, 10000).element_cost(1000).block_cost(200000));
    check_cover(blocks, 0, 10000);
    EXPECT_EQ(min_overhead, smallest(blocks));

    // A larger block_min is kept
    blocks = blocks_of(BoofConcurrency::loop_blocks(0, 10000).schedule(Schedule::Guided().block_min(min_time + 5))
            .element_cost(1000));
    EXPECT_EQ(min_time + 5, smallest(blocks));

    // Fixed::block_size() overrides it
    EXPECT_EQ(make_pair(0u, 3u), blocks_of(BoofConcurrency::loop_blocks(0, 10000)
            .schedule(Schedule::Fixed().block_size(3)).element_cost(1000)).at(0));
    EXPECT_THROW((void)BoofConcurrency::loop_blocks(0, 10).element_cost(0), invalid_argument);
}

/// single() runs with 1 thread, or when there's work for fewer than two blocks
TEST_F(BoofConcurrencyTest, single) {
    const BoofConcurrency::CostModel model;
    const uint32_t min_time = (uint32_t)(model.block_min_us*model.operations_per_us/1000);
    BoofConcurrency::set_max_threads(4);
    EXPECT_FALSE(runs_single(BoofConcurrency::loop_blocks(0, 10000).element_cost(1000)));
    EXPECT_FALSE(runs_single(BoofConcurrency::loop_blocks(0, 2*min_time).element_cost(1000)));
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 2*min_time - 1).element_cost(1000)));
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 1)));
    EXPECT_FALSE(runs_single(BoofConcurrency::loop_blocks(0, 2)));
    // An empty range runs nothing
    EXPECT_FALSE(runs_single(BoofConcurrency::loop_blocks(5, 5)));
    BoofConcurrency::set_max_threads(1);
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 10000).element_cost(1000)));

    // Without single(), run() runs every block on the calling thread
    BoofConcurrency::set_max_threads(4);
    auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 2*min_time - 1).element_cost(1000));
    check_cover(blocks, 0, 2*min_time - 1);
}

TEST_F(BoofConcurrencyTest, set_cost_model) {
    const BoofConcurrency::CostModel defaults;
    BoofConcurrency::CostModel model;
    model.operations_per_us = 7;
    model.block_min_us = 30;
    model.schedule_operations = 0;
    BoofConcurrency::set_cost_model(model);
    EXPECT_EQ(7u, BoofConcurrency::cost_model().operations_per_us);
    EXPECT_EQ(30u, BoofConcurrency::cost_model().block_min_us);
    EXPECT_EQ(0u, BoofConcurrency::cost_model().schedule_operations);
    // 210 operations per block, so 30 elements of 7
    BoofConcurrency::set_max_threads(4);
    auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 100).schedule(Schedule::Fixed()).element_cost(7));
    EXPECT_EQ(make_pair(0u, 30u), blocks.at(0));
    BoofConcurrency::set_cost_model(defaults);
}

/// testing_blocks() splits loops without a Fixed::block_size() into blocks of ceil(length/blocks), whatever their costs
TEST_F(BoofConcurrencyTest, testing_blocks) {
    BoofConcurrency::set_max_threads(4);
    BoofConcurrency::set_testing_blocks(4);
    EXPECT_EQ(4u, BoofConcurrency::testing_blocks());
    for (const auto& loop : {BoofConcurrency::loop_blocks(0, 10).element_cost(1).schedule(Schedule::Fixed().block_min(8)),
                             BoofConcurrency::loop_blocks(0, 10).element_cost(1).schedule(Schedule::Guided().block_min(8))}) {
        EXPECT_FALSE(runs_single(loop));
        EXPECT_EQ((vector<pair<uint32_t, uint32_t>>{{0, 3}, {3, 6}, {6, 9}, {9, 10}}), blocks_of(loop));
    }
    // More blocks than elements
    BoofConcurrency::set_testing_blocks(1000);
    auto blocks = blocks_of(BoofConcurrency::loop_blocks(0, 7));
    EXPECT_EQ(7u, blocks.size());
    check_cover(blocks, 0, 7);
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 1)));
    // Fixed::block_size() isn't changed
    EXPECT_EQ(make_pair(0u, 5u), blocks_of(BoofConcurrency::loop_blocks(0, 10)
            .schedule(Schedule::Fixed().block_size(5))).at(0));
    // 1 thread
    BoofConcurrency::set_max_threads(1);
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 10)));
    // Off
    BoofConcurrency::set_max_threads(4);
    BoofConcurrency::set_testing_blocks(0);
    EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 10).element_cost(1)));
}

/// testing_out_of_order() counts the concurrent loops split by testing_blocks() whose blocks finished out of sequence
TEST_F(BoofConcurrencyTest, testing_out_of_order) {
    BoofConcurrency::set_max_threads(4);
    BoofConcurrency::set_testing_blocks(4);
    // The first block finishes last, since the other threads run the rest while it sleeps
    auto first_last = []( uint32_t i0, uint32_t ) {
        if (i0 == 0)
            this_thread::sleep_for(chrono::milliseconds(50));
    };
    uint64_t count = BoofConcurrency::testing_out_of_order();
    BoofConcurrency::loop_blocks(0, 8).run(first_last);
    EXPECT_EQ(++count, BoofConcurrency::testing_out_of_order());

    // Each block waits for the one before it, so they finish in sequence
    atomic<uint32_t> done{0};
    BoofConcurrency::loop_blocks(0, 8).run([&]( uint32_t i0, uint32_t ) {
        while (done.load() != i0/2)
            this_thread::yield();
        done++;
    });
    EXPECT_EQ(count, BoofConcurrency::testing_out_of_order());

    // Not counted without testing blocks, or with one thread
    BoofConcurrency::set_testing_blocks(0);
    BoofConcurrency::loop_blocks(0, 8).schedule(Schedule::Fixed().block_size(2)).run(first_last);
    BoofConcurrency::set_testing_blocks(4);
    BoofConcurrency::set_max_threads(1);
    BoofConcurrency::loop_blocks(0, 8).run(first_last);
    EXPECT_EQ(count, BoofConcurrency::testing_out_of_order());
}

/// testing_loops() counts the loops split by testing_blocks() that the calling thread ran, whatever their threads
TEST_F(BoofConcurrencyTest, testing_loops) {
    auto nothing = []( uint32_t, uint32_t ) {};
    BoofConcurrency::set_testing_blocks(4);
    uint64_t count = BoofConcurrency::testing_loops();
    for (uint32_t threads : {4u, 1u}) {
        BoofConcurrency::set_max_threads(threads);
        BoofConcurrency::loop_blocks(0, 8).run(nothing);
        EXPECT_EQ(++count, BoofConcurrency::testing_loops()) << threads;
        BoofConcurrency::loop_blocks(0, 1).run(nothing);
        EXPECT_EQ(++count, BoofConcurrency::testing_loops()) << threads;
    }

    // Not counted for an empty range, a Fixed block_size() or without testing blocks
    BoofConcurrency::set_max_threads(4);
    BoofConcurrency::loop_blocks(5, 5).run(nothing);
    BoofConcurrency::loop_blocks(0, 8).schedule(Schedule::Fixed().block_size(2)).run(nothing);
    BoofConcurrency::set_testing_blocks(0);
    BoofConcurrency::loop_blocks(0, 8).run(nothing);
    EXPECT_EQ(count, BoofConcurrency::testing_loops());

    // Each thread has its own count
    BoofConcurrency::set_testing_blocks(4);
    std::thread([&] { BoofConcurrency::loop_blocks(0, 8).run(nothing); }).join();
    EXPECT_EQ(count, BoofConcurrency::testing_loops());
}

TEST_F(BoofConcurrencyTest, empty_range) {
    BoofConcurrency::set_max_threads(4);
    for (auto range : {make_pair(5u, 5u), make_pair(6u, 5u)}) {
        auto loop = BoofConcurrency::loop_blocks(range.first, range.second);
        EXPECT_TRUE(blocks_of(loop.schedule(Schedule::Fixed())).empty());
        EXPECT_TRUE(blocks_of(loop.schedule(Schedule::Guided())).empty());
    }
}

TEST_F(BoofConcurrencyTest, invalid_settings) {
    EXPECT_THROW((void)Schedule::Fixed().block_size(0), invalid_argument);
    EXPECT_THROW((void)Schedule::Fixed().block_min(0), invalid_argument);
    EXPECT_THROW((void)Schedule::Fixed().multiplier(0), invalid_argument);
    EXPECT_THROW((void)Schedule::Guided().block_min(0), invalid_argument);
    EXPECT_THROW((void)Schedule::Guided().multiplier(0), invalid_argument);
    // A Fixed block_size() can't be combined with the settings that size blocks from the range
    EXPECT_THROW((void)Schedule::Fixed().block_size(3).block_min(2), invalid_argument);
    EXPECT_THROW((void)Schedule::Fixed().block_size(3).multiplier(2), invalid_argument);
    EXPECT_THROW((void)Schedule::Fixed().block_min(2).block_size(3), invalid_argument);
    EXPECT_THROW((void)Schedule::Fixed().multiplier(2).block_size(3), invalid_argument);
}

/// A thread stuck on a slow block doesn't hold up the others, which do the remaining blocks
TEST_F(BoofConcurrencyTest, slow_block) {
    BoofConcurrency::set_max_threads(2);
    for (const auto& loop : {BoofConcurrency::loop_blocks(0, 40).schedule(Schedule::Fixed().block_size(1)),
                             BoofConcurrency::loop_blocks(0, 40).schedule(Schedule::Guided())}) {
        mutex m;
        vector<pair<uint32_t, thread::id>> owners;
        loop.run([&]( uint32_t i0, uint32_t ) {
            if (i0 == 0)
                this_thread::sleep_for(chrono::milliseconds(50));
            lock_guard<mutex> lock(m);
            owners.emplace_back(i0, this_thread::get_id());
        });
        sort(owners.begin(), owners.end());
        const thread::id slow = owners.at(0).second;
        EXPECT_EQ(1, count_if(owners.begin(), owners.end(), [&]( const auto& o ) { return o.second == slow; }));
    }
}

/// Every block runs at the same time: each waits until all of them have started
TEST_F(BoofConcurrencyTest, concurrent) {
    const uint32_t threads = BoofConcurrency::hardware_threads() + 3;
    BoofConcurrency::set_max_threads(threads);
    atomic<uint32_t> started{0};
    atomic<bool> all_started{true};
    auto loop = BoofConcurrency::loop_blocks(0, threads).schedule(Schedule::Fixed().block_size(1));
    loop.run([&]( uint32_t, uint32_t ) {
        started++;
        auto deadline = chrono::steady_clock::now() + chrono::seconds(10);
        while (started.load() < threads) {
            if (chrono::steady_clock::now() > deadline) {
                all_started = false;
                return;
            }
            this_thread::yield();
        }
    });
    EXPECT_TRUE(all_started.load());
}

/// An exception is rethrown after the running blocks finished, and skips the blocks that haven't started
TEST_F(BoofConcurrencyTest, exception) {
    for (uint32_t threads : {1u, 5u}) {
        BoofConcurrency::set_max_threads(threads);
        atomic<uint32_t> running{0}, started{0};
        auto loop = BoofConcurrency::loop_blocks(0, 1000).schedule(Schedule::Fixed().block_size(10));
        EXPECT_THROW(loop.run([&]( uint32_t i0, uint32_t ) {
            started++;
            running++;
            this_thread::sleep_for(chrono::milliseconds(2));
            running--;
            if (i0 == 30)
                throw runtime_error("block failed");
        }), runtime_error);
        EXPECT_EQ(0u, running.load());
        EXPECT_LT(started.load(), 100u);
        if (threads == 1)
            EXPECT_EQ(4u, started.load());

        // The pool still works afterwards
        EXPECT_EQ(100u, fixed_blocks(0, 1000, 10).size());
    }
}

/// A block can call loop_blocks(), even when every worker is busy
TEST_F(BoofConcurrencyTest, nested) {
    BoofConcurrency::set_max_threads(3);
    atomic<uint32_t> total{0};
    BoofConcurrency::loop_blocks(0, 6).run([&]( uint32_t i0, uint32_t i1 ) {
        for (uint32_t i = i0; i < i1; i++) {
            BoofConcurrency::loop_blocks(0, 10).run([&]( uint32_t j0, uint32_t j1 ) { total += j1 - j0; });
        }
    });
    EXPECT_EQ(60u, total.load());
}

/// A loop_blocks() inside a concurrent block runs single threaded on the block's thread
TEST_F(BoofConcurrencyTest, nested_single_threaded) {
    BoofConcurrency::set_max_threads(4);
    atomic<bool> correct{true};
    BoofConcurrency::loop_blocks(0, 4).run([&]( uint32_t, uint32_t ) {
        if (BoofConcurrency::available_threads() != 1)
            correct = false;
        auto inner = BoofConcurrency::loop_blocks(0, 100);
        if (!runs_single(inner))
            correct = false;
        const thread::id outer = this_thread::get_id();
        inner.run([&]( uint32_t, uint32_t ) {
            if (this_thread::get_id() != outer)
                correct = false;
        });
    });
    EXPECT_TRUE(correct.load());
    EXPECT_EQ(4u, BoofConcurrency::available_threads());

    // A single threaded outer loop doesn't limit the inner one
    BoofConcurrency::loop_blocks(0, 1).run([&]( uint32_t, uint32_t ) {
        EXPECT_EQ(4u, BoofConcurrency::available_threads());
    });
}

/// ThreadLimit limits the calling thread's calls until it's destroyed, and not other threads'
TEST_F(BoofConcurrencyTest, thread_limit) {
    BoofConcurrency::set_max_threads(4);
    {
        BoofConcurrency::ThreadLimit limit(1);
        EXPECT_EQ(1u, BoofConcurrency::available_threads());
        EXPECT_TRUE(runs_single(BoofConcurrency::loop_blocks(0, 100)));
        const thread::id caller = this_thread::get_id();
        atomic<bool> same{true};
        BoofConcurrency::loop_blocks(0, 100).run([&]( uint32_t, uint32_t ) {
            if (this_thread::get_id() != caller)
                same = false;
        });
        EXPECT_TRUE(same.load());

        // Another thread isn't limited
        uint32_t other = 0;
        thread([&] { other = BoofConcurrency::available_threads(); }).join();
        EXPECT_EQ(4u, other);

        // A limit inside it can only lower it
        {
            BoofConcurrency::ThreadLimit higher(3);
            EXPECT_EQ(1u, BoofConcurrency::available_threads());
        }
        EXPECT_EQ(1u, BoofConcurrency::available_threads());
    }
    {
        BoofConcurrency::ThreadLimit limit(3);
        EXPECT_EQ(3u, BoofConcurrency::available_threads());
        {
            BoofConcurrency::ThreadLimit lower(2);
            EXPECT_EQ(2u, BoofConcurrency::available_threads());
        }
        EXPECT_EQ(3u, BoofConcurrency::available_threads());
        // Not more than max_threads()
        BoofConcurrency::ThreadLimit more(9);
        BoofConcurrency::set_max_threads(2);
        EXPECT_EQ(2u, BoofConcurrency::available_threads());
    }
    EXPECT_EQ(2u, BoofConcurrency::available_threads());
    EXPECT_THROW(BoofConcurrency::ThreadLimit(0), invalid_argument);
}

/// Several threads call it at once, while the number of threads changes
TEST_F(BoofConcurrencyTest, concurrent_callers) {
    BoofConcurrency::set_max_threads(4);
    atomic<bool> correct{true};
    vector<thread> callers;
    for (int c = 0; c < 4; c++) {
        callers.emplace_back([&correct, c] {
            auto loop = BoofConcurrency::loop_blocks(0, 1000);
            if (c%2 == 0)
                loop = loop.schedule(Schedule::Fixed());
            for (int trial = 0; trial < 200; trial++) {
                vector<uint8_t> visited(1000, 0);
                loop.run([&]( uint32_t i0, uint32_t i1 ) {
                    for (uint32_t i = i0; i < i1; i++)
                        visited[i]++;
                });
                for (uint8_t v : visited) {
                    if (v != 1)
                        correct = false;
                }
            }
        });
    }
    for (uint32_t threads = 1; threads <= 6; threads++)
        BoofConcurrency::set_max_threads(threads);
    for (thread& caller : callers)
        caller.join();
    EXPECT_TRUE(correct.load());
}
