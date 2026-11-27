#ifndef BOOFCPP_PIXEL_MATH_H
#define BOOFCPP_PIXEL_MATH_H

#include <cmath>
#include <cstdint>
#include <type_traits>

#include "boofcv/image_types.h"
#include "boofcv/sanity_checks.h"

namespace boofcv {

    /// Per pixel math. The functions with the same name in BoofCV's PixelMath.
    class PixelMath {
    public:
        /// output = input*input
        static void pow2( const Gray<F32>& input, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            for (uint32_t y = 0; y < input.height; y++) {
                const F32* in = input.data + input.offset + y*input.stride;
                F32* out = output.data + output.offset + y*output.stride;
                for (uint32_t x = 0; x < input.width; x++)
                    out[x] = in[x]*in[x];
            }
        }

        /// Standard deviation from the mean and the mean of the squared values: sqrt(max(0, pow2 - mean^2))
        static void stdev( const Gray<F32>& mean, const Gray<F32>& pow2, Gray<F32>& stdev ) {
            checkSameShape(mean, pow2);
            stdev.reshape(mean.width, mean.height);
            for (uint32_t y = 0; y < mean.height; y++) {
                const F32* m = mean.data + mean.offset + y*mean.stride;
                const F32* p = pow2.data + pow2.offset + y*pow2.stride;
                F32* out = stdev.data + stdev.offset + y*stdev.stride;
                for (uint32_t x = 0; x < mean.width; x++) {
                    F32 v = p[x] - m[x]*m[x];
                    // NaN stays NaN
                    if (!(v != v) && v < 0)
                        v = 0;
                    out[x] = (F32)std::sqrt((double)v);
                }
            }
        }

        /// output = input + value
        template<class E>
        static void plus( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            for (uint32_t y = 0; y < input.height; y++) {
                const E* in = input.data + input.offset + y*input.stride;
                F32* out = output.data + output.offset + y*output.stride;
                for (uint32_t x = 0; x < input.width; x++)
                    out[x] = (F32)in[x] + value;
            }
        }

        /// output = input - value
        template<class E>
        static void minus( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            for (uint32_t y = 0; y < input.height; y++) {
                const E* in = input.data + input.offset + y*input.stride;
                F32* out = output.data + output.offset + y*output.stride;
                for (uint32_t x = 0; x < input.width; x++)
                    out[x] = (F32)in[x] - value;
            }
        }

        /// output = input*value
        template<class E>
        static void multiply( const Gray<E>& input, F32 value, Gray<F32>& output ) {
            output.reshape(input.width, input.height);
            for (uint32_t y = 0; y < input.height; y++) {
                const E* in = input.data + input.offset + y*input.stride;
                F32* out = output.data + output.offset + y*output.stride;
                for (uint32_t x = 0; x < input.width; x++)
                    out[x] = (F32)in[x]*value;
            }
        }
    };
}

#endif
