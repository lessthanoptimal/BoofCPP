#ifndef BOOFCPP_CONVOLVE_MT_H
#define BOOFCPP_CONVOLVE_MT_H

#include "boofcv/concurrency.h"
#include "boofcv/convolve.h"

#include <algorithm>

namespace boofcv {

    /// ConvolveImage with the output split into blocks of rows, which threads take until none are left:
    ///
    ///     for each block of output rows, concurrently:
    ///         ConvolveImage_Inner on the sub-image of the block's inner rows and the rows they read
    ///         ConvolveImage_Border on the block's border pixels
    ///
    /// Each output pixel is computed by the same code as ConvolveImage, so the results are identical for any number
    /// of threads
    class ConvolveImage_MT {
    public:
        template<class E, class R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            auto single = [&] { ConvolveImage::horizontal(kernel, input, output); };
            // The concurrent code can't handle a kernel as wide as the image
            if( kernel.width >= output.width ) {
                single();
                return;
            }
            const uint32_t right = output.width - (kernel.width - kernel.offset - 1);
            auto loop = BoofConcurrency::loop_blocks(0, output.height).element_cost(row_cost(output, kernel.width));
            loop.single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                inner_block(*input.image, output, 0, 0, y0, y1, [&kernel]( const Gray<E>& in, Gray<R>& out ) {
                    ConvolveImage_Inner::horizontal_unrolled(kernel, in, out); });
                ConvolveImage_Border::horizontal(kernel, input, output, 0, kernel.offset, y0, y1);
                ConvolveImage_Border::horizontal(kernel, input, output, right, output.width, y0, y1);
            });
        }

        template<class E, class R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            auto single = [&] { ConvolveImage::vertical(kernel, input, output); };
            // The concurrent code can't handle a kernel as tall as the image
            if( kernel.width >= output.height ) {
                single();
                return;
            }
            const uint32_t before = kernel.offset, after = kernel.width - kernel.offset - 1;
            auto loop = BoofConcurrency::loop_blocks(0, output.height).element_cost(row_cost(output, kernel.width));
            loop.single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                inner_block(*input.image, output, before, after, y0, y1,
                            [&kernel]( const Gray<E>& in, Gray<R>& out ) {
                    ConvolveImage_Inner::vertical_unrolled(kernel, in, out); });
                border_rows(output.height, before, after, y0, y1, [&]( uint32_t a, uint32_t b ) {
                    ConvolveImage_Border::vertical(kernel, input, output, 0, output.width, a, b); });
            });
        }

        template<class E, class R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            auto single = [&] { ConvolveImage::convolve(kernel, input, output); };
            // The concurrent code can't handle a kernel as wide or tall as the image
            if( kernel.width >= output.width || kernel.width >= output.height ) {
                single();
                return;
            }
            const uint32_t before = kernel.offset, after = kernel.width - kernel.offset - 1;
            const uint32_t right = output.width - after;
            auto loop = BoofConcurrency::loop_blocks(0, output.height)
                    .element_cost(row_cost(output, kernel.width*kernel.width));
            loop.single(single).run([&]( uint32_t y0, uint32_t y1 ) {
                inner_block(*input.image, output, before, after, y0, y1,
                            [&kernel]( const Gray<E>& in, Gray<R>& out ) {
                    ConvolveImage_Inner::convolve(kernel, in, out); });
                ConvolveImage_Border::convolve(kernel, input, output, 0, before, y0, y1);
                ConvolveImage_Border::convolve(kernel, input, output, right, output.width, y0, y1);
                border_rows(output.height, before, after, y0, y1, [&]( uint32_t a, uint32_t b ) {
                    ConvolveImage_Border::convolve(kernel, input, output, before, right, a, b); });
            });
        }

        /// Operations in a row of 'output': a multiply-add per kernel element and pixel
        static uint64_t row_cost( const ImageBase& output, uint32_t kernel_elements ) {
            return std::max<uint64_t>(1, (uint64_t)output.width*kernel_elements);
        }

        /// Calls inner(in, out) for the rows [y0, y1) that are inside [before, height - after), the rows the inner
        /// convolution writes. 'in' and 'out' are sub-images of those rows plus the 'before' rows above and 'after'
        /// rows below them, which it reads but doesn't write
        template<class E, class R, class Inner>
        static void inner_block( const Gray<E>& input, Gray<R>& output, uint32_t before, uint32_t after,
                                 uint32_t y0, uint32_t y1, Inner inner )
        {
            const uint32_t a = std::max(y0, before), b = std::min(y1, input.height - after);
            if( a >= b )
                return;
            const Gray<E> in = input.makeSubimage(0, a - before, input.width, b + after);
            Gray<R> out = output.makeSubimage(0, a - before, output.width, b + after);
            inner(in, out);
        }

        /// Calls rows(a, b) for the rows [y0, y1) that are in the top 'before' or bottom 'after' rows of the image
        template<class Rows>
        static void border_rows( uint32_t height, uint32_t before, uint32_t after, uint32_t y0, uint32_t y1,
                                 Rows rows )
        {
            if( y0 < before )
                rows(y0, std::min(y1, before));
            if( y1 > height - after )
                rows(std::max(y0, height - after), y1);
        }
    };
}

#endif
