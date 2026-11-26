#ifndef BOOFCPP_PIXEL_MATH_MT_H
#define BOOFCPP_PIXEL_MATH_MT_H

#include "boofcv/concurrency.h"
#include "boofcv/image_convert.h"
#include "boofcv/pixel_math.h"

namespace boofcv {

    /// PixelMath with the rows split into blocks, which threads take until none are left:
    ///
    ///     for each block of rows, concurrently:  PixelMath's code on the block's rows
    ///
    /// Every pixel is computed the same way, so the results are identical to PixelMath for any number of threads
    class PixelMath_MT {
    public:
        static void pow2( const Gray<F32>& input, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            rows(input, [&] { PixelMath::pow2(input, output); }, [&]( const Gray<F32>& in, Gray<F32>& out ) {
                PixelMath::pow2(in, out);
            }, output);
        }

        static void stdev( const Gray<F32>& mean, const Gray<F32>& pow2, Gray<F32>& stdev ) {
            checkSameShape(mean, pow2);
            stdev.reshape(mean.width, mean.height);
            BoofConcurrency::loop_blocks(0, mean.height)
                    .element_cost(std::max<uint64_t>(1, mean.width))
                    .single([&] { PixelMath::stdev(mean, pow2, stdev); })
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        Gray<F32> out = stdev.makeSubimage(0, y0, stdev.width, y1);
                        PixelMath::stdev(mean.makeSubimage(0, y0, mean.width, y1),
                                         pow2.makeSubimage(0, y0, pow2.width, y1), out);
                    });
        }

        template<class E>
        static void plus( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            rows(input, [&] { PixelMath::plus(input, value, output); }, [&]( const Gray<E>& in, Gray<F32>& out ) {
                PixelMath::plus(in, value, out);
            }, output);
        }

        template<class E>
        static void minus( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            rows(input, [&] { PixelMath::minus(input, value, output); }, [&]( const Gray<E>& in, Gray<F32>& out ) {
                PixelMath::minus(in, value, out);
            }, output);
        }

        template<class E>
        static void multiply( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            rows(input, [&] { PixelMath::multiply(input, value, output); }, [&]( const Gray<E>& in, Gray<F32>& out ) {
                PixelMath::multiply(in, value, out);
            }, output);
        }

    private:
        /// Calls 'block' with the input and output rows of each block. 'output' has the input's shape
        template<class I, class O, class Single, class Block>
        static void rows( const Gray<I>& input, Single single, Block block, Gray<O>& output ) {
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .single(single)
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        Gray<O> out = output.makeSubimage(0, y0, output.width, y1);
                        block(input.makeSubimage(0, y0, input.width, y1), out);
                    });
        }
    };

    /// convert() with the rows split into blocks, which threads take until none are left. Every pixel is converted the
    /// same way, so the results are identical to convert() for any number of threads
    class ConvertImage_MT {
    public:
        template<typename I, typename O>
        static void convert( const Gray<I>& input, Gray<O>& output ) {
            output.reshape(input.width, input.height);
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost(std::max<uint64_t>(1, input.width))
                    .single([&] { boofcv::convert(input, output); })
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        Gray<O> out = output.makeSubimage(0, y0, output.width, y1);
                        boofcv::convert(input.makeSubimage(0, y0, input.width, y1), out);
                    });
        }
    };
}

#endif
