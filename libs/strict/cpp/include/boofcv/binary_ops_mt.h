#ifndef BOOFCPP_BINARY_OPS_MT_H
#define BOOFCPP_BINARY_OPS_MT_H

#include "boofcv/binary_ops.h"
#include "boofcv/concurrency.h"
#include "boofcv/image_blur_mt.h"
#include "boofcv/image_statistics_mt.h"

namespace boofcv {

    /// ThresholdOps with the rows split into blocks, which threads take until none are left:
    ///
    ///     for each block of rows, concurrently:  ThresholdOps' code on the block's rows
    ///
    /// Every pixel is computed the same way, so the results are identical to ThresholdOps for any number of threads
    class ThresholdOps_MT {
    public:
        /// ThresholdOps::computeOtsu() with the histogram from ImageStatistics_MT
        template<class T>
        static uint32_t computeOtsu( const Gray<T>& input, double min_value, double max_value, bool otsu2 = false ) {
            return ThresholdOps::computeOtsu<T, ImageStatistics_MT>(input, min_value, max_value, otsu2);
        }

        template<class T, class TH = T>
        static void threshold( const Gray<T>& input, TH threshold, bool down, Gray<U8>& output ) {
            output.reshape(input.width, input.height);
            rows(input, output, [&] { ThresholdOps::threshold(input, threshold, down, output); },
                 [&]( const Gray<T>& in, Gray<U8>& out, uint32_t, uint32_t ) {
                     ThresholdOps::threshold(in, threshold, down, out);
                 });
        }

        /// ThresholdOps::threshold_double()
        template<class T>
        static void threshold_double( const Gray<T>& input, double threshold_d, bool down, Gray<U8>& output ) {
            if constexpr (std::is_integral_v<T>) {
                const auto threshold = static_cast<int64_t>(threshold_d);
                if (threshold < std::numeric_limits<T>::min()) {
                    fill(input, static_cast<U8>(down ? 0 : 1), output);
                } else if (threshold >= std::numeric_limits<T>::max()) {
                    fill(input, static_cast<U8>(down ? 1 : 0), output);
                } else {
                    ThresholdOps_MT::threshold(input, static_cast<T>(threshold), down, output);
                }
            } else {
                ThresholdOps_MT::threshold(input, static_cast<float>(threshold_d), down, output);
            }
        }

        /// ThresholdOps::localMean(). U8 splits ThresholdOps::localMean_U8_rows() into blocks of rows, each of which
        /// sums the rows of its first window from scratch. Other types use BlurImageOps_MT::mean()
        template<class T>
        static void localMean( const Gray<T>& input, Gray<U8>& output, const ConfigLength& width, float scale,
                               bool down, Gray<T>& storage1, Gray<T>& storage2 ) {
            output.reshape(input.width, input.height);
            const uint32_t radius = (uint32_t)width.computeI(std::min(input.width, input.height))/2;
            if constexpr (std::is_same_v<T, U8>) {
                if (radius == 0)
                    throw std::invalid_argument("Radius must be > 0");
                if (input.width == 0 || input.height == 0)
                    return;
                // A block also computes the horizontal means of the 2*radius + 1 rows of its first window
                const uint64_t window = 2*(uint64_t)std::min(radius, input.height) + 1;
                BoofConcurrency::loop_blocks(0, input.height)
                        .element_cost(2*std::max<uint64_t>(1, input.width))
                        .block_cost(window*input.width)
                        .single([&] { ThresholdOps::localMean_U8_rows(input, output, radius, scale, down, 0,
                                                                      input.height); })
                        .run([&]( uint32_t y0, uint32_t y1 ) {
                            ThresholdOps::localMean_U8_rows(input, output, radius, scale, down, y0, y1);
                        });
            } else {
                storage1.reshape(input.width, input.height);
                storage2.reshape(input.width, input.height);
                BlurImageOps_MT::mean(input, storage1, radius, radius, storage2);
                rows(input, output, [&] { ThresholdOps::threshold_local(input, storage1, scale, down, output); },
                     [&]( const Gray<T>& in, Gray<U8>& out, uint32_t y0, uint32_t y1 ) {
                         ThresholdOps::threshold_local(in, storage1.makeSubimage(0, y0, storage1.width, y1), scale,
                                                       down, out);
                     });
            }
        }

    private:
        /// 'output' reshaped to the input and filled with 'value'
        template<class T>
        static void fill( const Gray<T>& input, U8 value, Gray<U8>& output ) {
            output.reshape(input.width, input.height);
            rows(input, output, [&] { ImageMiscOps::fill(output, value); },
                 [&]( const Gray<T>&, Gray<U8>& out, uint32_t, uint32_t ) { ImageMiscOps::fill(out, value); });
        }

        /// Calls block(input rows, output rows, y0, y1) for each block. 'output' has the input's shape
        template<class T, class Single, class Block>
        static void rows( const Gray<T>& input, Gray<U8>& output, Single single, Block block ) {
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .single(single)
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        Gray<U8> out = output.makeSubimage(0, y0, output.width, y1);
                        block(input.makeSubimage(0, y0, input.width, y1), out, y0, y1);
                    });
        }
    };
}

#endif
