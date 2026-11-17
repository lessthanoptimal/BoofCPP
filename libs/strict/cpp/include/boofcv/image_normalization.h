#ifndef BOOFCPP_IMAGE_NORMALIZATION_H
#define BOOFCPP_IMAGE_NORMALIZATION_H

#include <cmath>
#include <cstdint>
#include <type_traits>

#include "boofcv/image_types.h"
#include "boofcv/pixel_math.h"

namespace boofcv {

    /// Parameters used to normalize an image: output = (input + offset)/divisor. Same as BoofCV's
    /// NormalizeParameters
    struct NormalizeParameters {
        double offset = 0;
        double divisor = 1;
    };

    /// Normalizes images to improve numerical stability. Same as BoofCV's ImageNormalization, including the order of
    /// the floating point operations. Statistics are summed like BoofCV's single threaded code.
    class ImageNormalization {
    public:
        /// Scales the image so that it has a mean of zero and a maximum absolute value of one.
        /// Same as BoofCV's ImageNormalization.zeroMeanMaxOne(). If every pixel is zero, 'output' is only reshaped.
        ///
        /// @param parameters The offset and divisor that were applied
        template<class E>
        static void zero_mean_max_one( const Gray<E>& input, Gray<F32>& output, NormalizeParameters& parameters ) {
            output.reshape(input.width, input.height);

            // Numerical errors are a concern and if you sum up the input it could overflow
            const double scale = (double)max_abs(input);
            if (scale == 0.0) {
                parameters.offset = 0.0;
                parameters.divisor = 1.0;
                return;
            }
            PixelMath::multiply(input, (F32)(1.0/scale), output);
            // work with the scaled image
            const double mean = (double)mean_F32(output);
            PixelMath::minus(output, (F32)mean, output);
            double scale2;
            if (std::is_signed<E>::value) {
                scale2 = (double)max_abs(output);
            } else {
                // image is scaled from 0 to 1.0
                scale2 = mean < 0.5 ? 1.0 - mean : mean;
            }
            if (scale2 != 0.0)
                PixelMath::multiply(output, (F32)(1.0/scale2), output);
            else
                scale2 = 1.0;

            parameters.offset = -mean*scale;
            parameters.divisor = scale*scale2;
        }

        /// Applies normalization parameters to an image. Same as BoofCV's ImageNormalization.apply()
        template<class E>
        static void apply( const Gray<E>& input, const NormalizeParameters& parameters, Gray<F32>& output ) {
            PixelMath::plus(input, (F32)parameters.offset, output);
            PixelMath::multiply(output, (F32)(1.0/parameters.divisor), output);
        }

        /// Largest absolute pixel value. Same as BoofCV's ImageStatistics.maxAbs()
        template<class E>
        static E max_abs( const Gray<E>& input ) {
            E output = abs_value(input.data[input.offset]);
            for (uint32_t y = 0; y < input.height; y++) {
                const E* row = input.data + input.offset + y*input.stride;
                for (uint32_t x = 0; x < input.width; x++) {
                    E v = abs_value(row[x]);
                    if (v > output)
                        output = v;
                }
            }
            return output;
        }

        /// Mean summed in a single float, row by row. Same as BoofCV's single threaded ImageStatistics.mean(GrayF32)
        static F32 mean_F32( const Gray<F32>& input ) {
            F32 total = 0;
            for (uint32_t y = 0; y < input.height; y++) {
                const F32* row = input.data + input.offset + y*input.stride;
                for (uint32_t x = 0; x < input.width; x++)
                    total += row[x];
            }
            return total/(F32)(int32_t)(input.width*input.height);
        }

    private:
        template<class E>
        static E abs_value( E v ) {
            if constexpr (std::is_unsigned<E>::value)
                return v;
            else
                return std::abs(v);
        }
    };
}

#endif
