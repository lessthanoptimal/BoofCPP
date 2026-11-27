#ifndef BOOFCPP_IMAGE_STATISTICS_MT_H
#define BOOFCPP_IMAGE_STATISTICS_MT_H

#include <mutex>

#include "boofcv/concurrency.h"
#include "boofcv/image_statistics.h"

namespace boofcv {

    /// ImageStatistics with the rows split into blocks, which threads take until none are left
    class ImageStatistics_MT {
    public:
        /// ImageStatistics::histogram() of each block of rows, added together:
        ///
        ///     for each block of rows, concurrently:  histogram of the block's rows, added to 'histogram'
        ///
        /// Counts don't depend on the order they're added in, so the results are identical for any number of threads
        template<class T, class V>
        static void histogram( const Gray<T>& input, V minValue, GrowArray<uint32_t>& histogram ) {
            std::mutex lock;
            histogram.fill(0);
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .block_cost(2*(uint64_t)histogram.size)
                    .single([&] { ImageStatistics::histogram(input, minValue, histogram); })
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        GrowArray<uint32_t> block(histogram.size);
                        ImageStatistics::histogram(input.makeSubimage(0, y0, input.width, y1), minValue, block);
                        std::lock_guard<std::mutex> guard(lock);
                        for (uint32_t i = 0; i < histogram.size; i++)
                            histogram[i] += block[i];
                    });
        }
    };
}

#endif
