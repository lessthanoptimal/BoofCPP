#ifndef BOOFCPP_IMAGE_NORMALIZATION_MT_H
#define BOOFCPP_IMAGE_NORMALIZATION_MT_H

#include <algorithm>
#include <mutex>
#include <vector>

#include "boofcv/concurrency.h"
#include "boofcv/image_normalization.h"
#include "boofcv/pixel_math_mt.h"

namespace boofcv {

    /// ImageNormalization with the rows split into blocks, which threads take until none are left:
    ///
    ///     max_abs:   for each block of rows, concurrently:  largest absolute value, starting from the first pixel's
    ///                then the largest of the blocks
    ///     mean_F32:  for each block of rows, concurrently:  each row's total
    ///                then the row totals added in row order
    ///     scaling:   PixelMath_MT
    ///
    /// The maximum doesn't depend on the order, and the mean adds the same row totals in the same order, so the results
    /// are identical to ImageNormalization for any number of threads
    class ImageNormalization_MT {
    public:
        /// ImageNormalization::zero_mean_max_one()
        template<class E>
        static void zero_mean_max_one( const Gray<E>& input, Gray<F32>& output, NormalizeParameters& parameters ) {
            output.reshape(input.width, input.height);

            const double scale = (double)max_abs(input);
            if (scale == 0.0) {
                parameters.offset = 0.0;
                parameters.divisor = 1.0;
                return;
            }
            PixelMath_MT::multiply(input, (F32)(1.0/scale), output);
            const double mean = (double)mean_F32(output);
            PixelMath_MT::minus(output, (F32)mean, output);
            double scale2;
            if (std::is_signed<E>::value) {
                scale2 = (double)max_abs(output);
            } else {
                scale2 = mean < 0.5 ? 1.0 - mean : mean;
            }
            if (scale2 != 0.0)
                PixelMath_MT::multiply(output, (F32)(1.0/scale2), output);
            else
                scale2 = 1.0;

            parameters.offset = -mean*scale;
            parameters.divisor = scale*scale2;
        }

        /// ImageNormalization::apply()
        template<class E>
        static void apply( const Gray<E>& input, const NormalizeParameters& parameters, Gray<F32>& output ) {
            PixelMath_MT::plus(input, (F32)parameters.offset, output);
            PixelMath_MT::multiply(output, (F32)(1.0/parameters.divisor), output);
        }

        /// ImageNormalization::max_abs()
        template<class E>
        static E max_abs( const Gray<E>& input ) {
            const E first = ImageNormalization::abs_value(input.data[input.offset]);
            std::mutex lock;
            E output = first;
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .single([&] { output = ImageNormalization::max_abs(input); })
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        // starts from the first pixel, like the single threaded code, so a NaN is never chosen
                        E block = first;
                        for (uint32_t y = y0; y < y1; y++) {
                            const E* row = input.data + input.offset + y*input.stride;
                            for (uint32_t x = 0; x < input.width; x++) {
                                E v = ImageNormalization::abs_value(row[x]);
                                if (v > block)
                                    block = v;
                            }
                        }
                        std::lock_guard<std::mutex> guard(lock);
                        if (block > output)
                            output = block;
                    });
            return output;
        }

        /// ImageNormalization::mean_F32()
        static F32 mean_F32( const Gray<F32>& input ) {
            std::vector<F32> row_totals(input.height);
            bool single = false;
            F32 single_mean = 0;
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .single([&] {
                        single = true;
                        single_mean = ImageNormalization::mean_F32(input);
                    })
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        for (uint32_t y = y0; y < y1; y++)
                            row_totals[y] = ImageNormalization::row_total(input, y);
                    });
            if (single)
                return single_mean;
            F32 total = 0;
            for (uint32_t y = 0; y < input.height; y++)
                total += row_totals[y];
            return total/(F32)(int32_t)(input.width*input.height);
        }
    };
}

#endif
