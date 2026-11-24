#ifndef BOOFCPP_IMAGE_BLUR_MT_H
#define BOOFCPP_IMAGE_BLUR_MT_H

#include "boofcv/concurrency.h"
#include "boofcv/image_blur.h"

#include <algorithm>
#include <type_traits>
#include <vector>

namespace boofcv {

    /// ConvolveImageMean for integer images, with the output split into blocks of rows that threads take until none
    /// are left:
    ///
    ///     horizontal:  for each block of rows, concurrently:  ConvolveImageMean's code on the block's rows
    ///     vertical:    for each block of rows, concurrently:
    ///                      column sums of the 'length' rows above the block's first inner row, then running sums
    ///                      the block's top or bottom border rows
    ///
    /// Each vertical block restarts its running sums, which gives the same sums only for integers, so the results are
    /// identical to ConvolveImageMean for any number of threads
    class ConvolveImageMean_MT {
    public:
        template< class E>
        static void horizontal( const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            static_assert(std::is_integral<E>::value, "Floating point sums depend on the blocks");
            checkSameShape(input,output);

            auto single = [&] { ConvolveImageMean::horizontal(input, output, radius); };
            // The concurrent code can't handle a kernel wider than the image
            if( radius*2 + 1 > input.width ) {
                single();
                return;
            }
            typedef typename TypeInfo<E>::signed_type signed_type;
            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            rows(input, 0).single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                const Gray<E> in = input.makeSubimage(0, y0, input.width, y1);
                Gray<E> out = output.makeSubimage(0, y0, output.width, y1);
                ConvolveNormalized_JustBorder::horizontal(kernel, in, out);
                ConvolveImageMean::inner_horizontal(in, out, radius);
            });
        }

        template< class E>
        static void vertical( const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            static_assert(std::is_integral<E>::value, "Floating point sums depend on the blocks");
            checkSameShape(input,output);

            auto single = [&] { ConvolveImageMean::vertical(input, output, radius); };
            // The concurrent code can't handle a kernel taller than the image
            if( radius*2 + 1 > input.height ) {
                single();
                return;
            }
            typedef typename TypeInfo<E>::signed_type signed_type;
            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            rows(input, radius*2 + 1).single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                ConvolveNormalized_JustBorder::vertical(kernel, input, output, y0, y1);
                // inner rows of the block, from the input rows 'radius' above and below them
                const uint32_t a = std::max(y0, radius), b = std::min(y1, input.height - radius);
                if( a >= b )
                    return;
                const Gray<E> in = input.makeSubimage(0, a - radius, input.width, b + radius);
                Gray<E> out = output.makeSubimage(0, a - radius, output.width, b + radius);
                ConvolveImageMean::inner_vertical(in, out, radius);
            });
        }

        /// ConvolveImageMean::horizontal() with a border
        template< class E>
        static void horizontal( const Gray<E>& input, Gray<E>& output, int32_t offset, int32_t length,
                                ImageBorder<E>& border ) {
            static_assert(std::is_integral<E>::value, "Floating point sums depend on the blocks");
            output.reshape(input.width, input.height);
            border.setImage(input);
            auto single = [&] { ConvolveImageMean::horizontal(input, output, offset, length, border); };
            rows(input, 0).single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                ConvolveImageMean::horizontal(input, output, offset, length, border, y0, y1);
            });
        }

        /// ConvolveImageMean::vertical() with a border
        template< class E>
        static void vertical( const Gray<E>& input, Gray<E>& output, int32_t offset, int32_t length,
                              ImageBorder<E>& border ) {
            static_assert(std::is_integral<E>::value, "Floating point sums depend on the blocks");
            output.reshape(input.width, input.height);
            border.setImage(input);
            auto single = [&] {
                std::vector<typename TypeInfo<E>::sum_type> totals;
                ConvolveImageMean::vertical(input, output, offset, length, border, totals);
            };
            rows(input, (uint32_t)length).single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                std::vector<typename TypeInfo<E>::sum_type> totals;
                ConvolveImageMean::vertical(input, output, offset, length, border, totals, y0, y1);
            });
        }

    private:
        /// Loop over the rows of 'image'. A row costs about 3 operations per pixel (subtract, add and divide), and a
        /// block first sums 'restart_rows' rows to restart its running sums
        static BoofConcurrency::LoopBlocks rows( const ImageBase& image, uint32_t restart_rows ) {
            return BoofConcurrency::loop_blocks(0, image.height)
                    .element_cost(std::max<uint64_t>(1, 3*(uint64_t)image.width))
                    .block_cost((uint64_t)restart_rows*image.width);
        }
    };

    /// BlurImageOps' mean filters for integer images, with ConvolveImageMean_MT. The results are identical to
    /// BlurImageOps for any number of threads
    class BlurImageOps_MT {
    public:
        /// BlurImageOps::mean()
        template<class E>
        static void mean( const Gray<E>& input, Gray<E>& output, uint32_t radius_x, uint32_t radius_y,
                          Gray<E>& storage ) {
            if (radius_x <= 0 || radius_y <= 0)
                throw invalid_argument("Radius must be > 0");

            output.reshape(input.width, input.height);
            storage.reshape(input.width, input.height);

            ConvolveImageMean_MT::horizontal(input, storage, radius_x);
            ConvolveImageMean_MT::vertical(storage, output, radius_y);
        }

        /// BlurImageOps::mean_border()
        template< class E>
        static void mean_border( const Gray<E>& input, Gray<E>& output, int32_t radius_x, int32_t radius_y,
                                 ImageBorder<E>& border, Gray<E>& storage ) {
            if (radius_x <= 0 || radius_y <= 0)
                throw invalid_argument("Radius must be > 0");
            storage.reshape(input.width, input.height);
            ConvolveImageMean_MT::horizontal(input, storage, radius_x, radius_x*2 + 1, border);
            ConvolveImageMean_MT::vertical(storage, output, radius_y, radius_y*2 + 1, border);
        }
    };
}

#endif
