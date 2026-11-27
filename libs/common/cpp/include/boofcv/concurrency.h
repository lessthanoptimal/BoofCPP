#ifndef BOOFCPP_CONCURRENCY_H
#define BOOFCPP_CONCURRENCY_H

#include <cstdint>
#include <functional>

namespace boofcv {

    /// How BoofConcurrency::loop_blocks() splits its range into blocks. Each schedule has its own settings, e.g.
    /// `schedule(Schedule::Fixed().block_size(16))`. Defined after BoofConcurrency
    struct Schedule {
        class Fixed;
        class Guided;
    };

    /// Process-wide thread pool. A call uses at most max_threads() threads, including the calling thread. Whenever a
    /// call would use only one thread, it calls the single() function instead, if one was given.
    ///
    /// Several threads can call it at once, e.g. two Java threads each running a native op. They share the workers,
    /// and can slow each other down. A call returns as soon as its own blocks are done.
    ///
    /// `loop_blocks(begin, end)` splits a range into blocks, which threads consume until none are left, see Schedule.
    /// Nested calls (calls to `loop_blocks()` from inside a block) run single threaded on the calling thread.
    /// Configuration is done with a fluent style:
    ///
    ///     loop_blocks(0, 1200).schedule(Schedule::Fixed().block_min(10)).
    ///         single([&] { foo(input, output); }).run([&]{ bar(input, output); });
    ///
    /// The default for each configuration is specified by the setter documentation. ThreadLimit limits the number of
    /// threads the calling thread's calls can use until it goes out of scope; other threads aren't affected. In the
    /// example below the current thread is limited to using at most 2 threads:
    ///
    ///     BoofConcurrency::ThreadLimit limit(2);
    ///
    /// Every variant (strict, fast) shares this pool, since it's in the common library they all link. In Java,
    /// NativeConcurrency.setMaxThreads() sets it. testing_blocks() is for tests only.
    class BoofConcurrency {
    public:
        /// Settings of a loop_blocks() call. run() runs it
        class LoopBlocks {
        public:
            /// Specifies the range of values it will process, which will then be broken up into blocks.
            LoopBlocks( uint32_t begin, uint32_t end ) : begin_(begin), end_(end) {}

            /// Default Schedule::Guided()
            [[nodiscard]] LoopBlocks& schedule( const Schedule::Fixed& fixed );
            [[nodiscard]] LoopBlocks& schedule( const Schedule::Guided& guided );

            /// Number of operations it takes to process a single element in the "range of values". Usage is
            /// scheduler dependent, but can determine if additional threads are needed or block size.
            ///
            /// Example: For a 1-D convolution with a kernel of width K, and the range set to rows, this will be K*width.
            /// One multiply-add per kernel element per pixel.
            ///
            /// Default: not set, so the cost model isn't used.
            ///
            /// @param operations Number of operations per element. Must be > 0, else throws std::invalid_argument.
            [[nodiscard]] LoopBlocks& element_cost( uint64_t operations );

            /// Work each block does before its elements, in operations, e.g. restarting a running sum. Default 0
            [[nodiscard]] LoopBlocks& block_cost( uint64_t operations );

            /// Called by run() instead of the blocks when it would use one thread: Typically when available_threads()
            /// is 1, or there's too little work for multiple blocks.
            [[nodiscard]] LoopBlocks& single( std::function<void()> single );

            /// Calls block(i0, i1) on each block of [begin, end). Up to available_threads() threads, including the
            /// calling thread, take the next block until none are left. Returns after every block is done. If a block
            /// throws, blocks that haven't started are skipped and one of the exceptions is rethrown
            void run( const std::function<void(uint32_t, uint32_t)>& block ) const;

        private:
            /// Block size, or smallest block with Guided, and the threads run() uses
            struct Plan {
                uint64_t size;
                uint32_t threads;
                bool guided;
                bool testing;   // split by testing_blocks()
            };
            Plan plan() const;

            uint32_t begin_, end_;
            uint64_t element_cost_ = 0;   // 0 if it isn't set
            uint64_t block_cost_ = 0;
            // The schedule's settings. block_size_ is 0 if it isn't set, and always with Guided
            bool guided_ = true;
            uint32_t block_size_ = 0;
            uint32_t block_min_ = 1;
            uint32_t multiplier_ = 4;
            std::function<void()> single_;
        };

        /// Converts LoopBlocks::element_cost() and block_cost() to block sizes
        struct CostModel {
            /// Operations per microsecond of one thread. Measured 3k-25k for BoofCPP's mean and convolution
            uint64_t operations_per_us = 10000;
            /// Fewest microseconds of work in a block. Waking 3 threads takes about 6 us
            uint64_t block_min_us = 20;
            /// Operations it costs to schedule a block, about 0.5 us
            uint64_t schedule_operations = 5000;
        };

        static CostModel cost_model();

        /// Changes cost_model(), e.g. so tests split small images into blocks
        static void set_cost_model( const CostModel& model );

        /// For tests only. If above 0, loops without a Fixed::block_size() are split into blocks of
        /// ceil(length/blocks) regardless of their costs and schedule, so the code that splits small images into
        /// blocks is tested. 0, the default, turns it off
        static uint32_t testing_blocks();

        static void set_testing_blocks( uint32_t blocks );

        /// For tests only. Number of concurrent loops split by testing_blocks() that the calling thread ran, whose
        /// blocks finished out of sequence. Tests use it to check that blocks ran out of order
        static uint64_t testing_out_of_order();

        /// For tests only. Number of concurrent loops split by testing_blocks() that the calling thread ran, even ones
        /// with a single block or thread. Tests use it to check that a call ran concurrent code
        static uint64_t testing_loops();

        /// Number of threads the system reports, or 1 if it doesn't report one
        static uint32_t hardware_threads();

        /// Most threads a call uses, including the calling thread. 1 runs the single threaded code. Defaults to
        /// hardware_threads()
        static uint32_t max_threads();

        /// Changes max_threads(). Throws std::invalid_argument if 'threads' is 0
        static void set_max_threads( uint32_t threads );

        /// Threads the calling thread's calls can use: max_threads(), at most its ThreadLimit, and 1 inside a block
        static uint32_t available_threads();

        /// Limits the threads of the calling thread's calls to 'threads' until it's destroyed. A ThreadLimit inside
        /// another one can only lower the limit. Throws std::invalid_argument if 'threads' is 0
        class ThreadLimit {
        public:
            explicit ThreadLimit( uint32_t threads );
            ~ThreadLimit();
            ThreadLimit( const ThreadLimit& ) = delete;
            ThreadLimit& operator=( const ThreadLimit& ) = delete;

        private:
            uint32_t previous;
        };

        /// Splits [begin, end) into blocks that threads run concurrently, e.g.
        /// `loop_blocks(0, height).element_cost(width).run([&]( uint32_t y0, uint32_t y1 ) { ... })`
        [[nodiscard]] static LoopBlocks loop_blocks( uint32_t begin, uint32_t end ) { return {begin, end}; }
    };

    /// Fixed-size blocks are assigned to threads in sequential order. Block boundaries are known in advance.
    ///
    /// Size of a block is set by `block_size` or `multiplier`, if neither is set then the default of block_min=1 and
    /// multiplier=4 is used. If specified by `block_size` then every block has that size. `multiplier` will set it
    /// based on this formula: `max(block_min, length/(max_threads()*multiplier))`
    /// If both block_size and (multiplier or block_min) are set then std::invalid_argument is thrown.
    ///
    /// If element_cost() is set, the cost model can raise the multiplier formula's result. It doesn't change block_size.
    class Schedule::Fixed {
    public:
        /// Size of every block. Throws std::invalid_argument if 0, or if block_min() or multiplier() was set
        [[nodiscard]] Fixed& block_size( uint32_t size );

        /// Smallest block. Default 1. Throws std::invalid_argument if 0, or if block_size() was set
        [[nodiscard]] Fixed& block_min( uint32_t size );

        /// Adjusts divisor when automatically setting block_size based on max_threads. Default
        /// is 4. Throws std::invalid_argument if 0, or if block_size() was set
        [[nodiscard]] Fixed& multiplier( uint32_t blocks_per_thread );
    private:
        friend class BoofConcurrency::LoopBlocks;
        uint32_t block_size_ = 0;   // 0 if it isn't set
        uint32_t block_min_ = 1;
        uint32_t multiplier_ = 4;
        bool sized_from_range_ = false;   // block_min() or multiplier() was set
    };

    /// Threads take blocks in sequential order, each sized from the remaining range:
    /// size = remaining/(multiplier*num_threads). Larger blocks first reduce the scheduling overhead and improve
    /// memory locality, and the smaller ones at the end let faster cores process more. The size shrinks until it hits
    /// block_min() or the cost model's minimum, whichever is larger.
    class Schedule::Guided {
    public:
        /// Smallest block. Default 1. Throws std::invalid_argument if 0
        [[nodiscard]] Guided& block_min( uint32_t size );

        /// The first block is 1/(threads*multiplier) of the range. Default 4, so threads on cores up to about
        /// 2*multiplier - 1 times slower don't hold up the others. Throws std::invalid_argument if 0
        [[nodiscard]] Guided& multiplier( uint32_t blocks_per_thread );

    private:
        friend class BoofConcurrency::LoopBlocks;
        uint32_t block_min_ = 1;
        uint32_t multiplier_ = 4;
    };
}

#endif
