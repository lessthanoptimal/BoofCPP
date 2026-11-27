#ifndef BOOFCPP_TESTING_CONCURRENCY_H
#define BOOFCPP_TESTING_CONCURRENCY_H

#include "testing_utils.h"
#include "boofcv/concurrency.h"

#include <functional>
#include <string>

namespace boofcv {

    /// Fixture for the tests of _MT code, which compare it to the single threaded code. Restores the threads and
    /// testing blocks after each test
    class ConcurrentTest : public BoofCPPTest {
    protected:
        const uint32_t original_threads = BoofConcurrency::max_threads();

        void TearDown() override {
            BoofConcurrency::set_max_threads(original_threads);
            BoofConcurrency::set_testing_blocks(0);
        }

        /// More threads than cores
        static uint32_t many_threads() { return BoofConcurrency::hardware_threads() + 3; }

        /// Calls 'compare' with more threads than cores and testing blocks, which split small images into a few
        /// blocks, then one for each row. 'compare' runs the single threaded and _MT code and compares them
        static void for_each_setting( const std::function<void()>& compare ) {
            for (uint32_t blocks : {3u, 1000u}) {
                BoofConcurrency::set_max_threads(many_threads());
                BoofConcurrency::set_testing_blocks(blocks);
                SCOPED_TRACE("threads " + std::to_string(many_threads()) + " blocks " + std::to_string(blocks));
                compare();
            }
        }

        /// Calls 'compare(scale)' for scale 1, 2, 4, ... until its blocks finish out of sequence, with more threads
        /// than cores and one block for each row. 'compare' runs the single threaded and _MT code on images 'scale'
        /// times larger and compares them. Fails if blocks never finish out of sequence, unless there's one core
        static void check_blocks_out_of_order( const std::function<void(uint32_t scale)>& compare,
                                               uint32_t max_scale = 64 ) {
            BoofConcurrency::set_max_threads(many_threads());
            BoofConcurrency::set_testing_blocks(1000);
            const uint64_t before = BoofConcurrency::testing_out_of_order();
            for (uint32_t scale = 1; scale <= max_scale; scale *= 2) {
                SCOPED_TRACE("out of order, scale " + std::to_string(scale));
                compare(scale);
                if (BoofConcurrency::testing_out_of_order() > before)
                    return;
            }
            if (BoofConcurrency::hardware_threads() == 1)
                GTEST_SKIP() << "One core, so blocks might not run out of sequence";
            ADD_FAILURE() << "Blocks never finished out of sequence, up to scale " << max_scale;
        }

        /// Sub-image of a larger image, so its offset and stride aren't the defaults. The parent owns the data
        template<class E>
        static Gray<E> subimage( Gray<E>& parent, uint32_t width, uint32_t height ) {
            parent.reshape(width + 4, height + 6);
            return parent.makeSubimage(2, 3, 2 + width, 3 + height);
        }
    };
}

#endif
