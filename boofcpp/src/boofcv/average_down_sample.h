#ifndef BOOFCPP_AVERAGE_DOWN_SAMPLE_H
#define BOOFCPP_AVERAGE_DOWN_SAMPLE_H

#include "convolve.h"
#include "image_types.h"
#include <algorithm>
#include <type_traits>

namespace boofcv {
    /// Signed total/count rounded to the nearest integer, half away from zero. 'half' is count/2. A branch on the sign
    /// is slow when pixels have both signs
    template<class T>
    T divide_round_away( T total, T count, T half ) {
        const T sign = total >> (sizeof(T)*8 - 1);
        // total >= 0 ? (total + half)/count : (total - half)/count
        return (total + ((half ^ sign) - sign))/count;
    }

    /// Mean of the pixels in a region with 'count' pixels, the same as BoofCV. Floats divide by 'count' and integers
    /// round to the nearest value, half away from zero
    template<class E>
    class RegionMean {
    public:
        typedef typename TypeInfo<E>::sum_type sum_type;

        explicit RegionMean( uint32_t count ) : divisor(count), count((sum_type)count), half((sum_type)(count/2)) {}

        E operator()( sum_type total ) const {
            if constexpr (std::is_integral<E>::value && std::is_signed<E>::value) {
                return static_cast<E>(divide_round_away(total, count, half));
            } else {
                return divisor(total);
            }
        }

    private:
        MeanDivisor<E> divisor;
        sum_type count;
        sum_type half;
    };

    /// Down samples an image by averaging the pixels inside square regions, the same as BoofCV's AverageDownSampleOps.
    /// The regions are 'width' pixels wide and start at (0,0). Regions along the right and bottom borders only
    /// include the pixels inside the image.
    class AverageDownSampleOps {
    public:
        /// Length of a side in the down sampled image
        static uint32_t down_sample_size( uint32_t length, uint32_t width ) {
            return length/width + (length%width != 0 ? 1 : 0);
        }

        /// Averages square regions which are 'width' pixels wide. The output is reshaped to the down sampled size.
        template<class E>
        static void down( const Gray<E>& input, uint32_t width, Gray<E>& output ) {
            if (width == 0)
                throw std::invalid_argument("width must be more than zero");

            output.reshape(down_sample_size(input.width, width), down_sample_size(input.height, width));

            if (width == 2)
                down2(input, output);
            else
                down_n(input, width, output);
        }

    private:
        /// Regions which are 2 pixels wide, the same as BoofCV's ImplAverageDownSample2
        template<class E>
        static void down2( const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::sum_type sum_type;

            const uint32_t max_x = input.width - input.width%2;
            const uint32_t max_y = input.height - input.height%2;
            const RegionMean<E> mean4(4), mean2(2);

            for (uint32_t y = 0; y < max_y; y += 2) {
                const E* row0 = &input.data[input.offset + y*input.stride];
                const E* row1 = row0 + input.stride;
                E* out = &output.data[output.offset + (y/2)*output.stride];

                for (uint32_t x = 0; x < max_x; x += 2) {
                    sum_type total = row0[x];
                    total += row0[x + 1];
                    total += row1[x];
                    total += row1[x + 1];
                    out[x/2] = mean4(total);
                }

                if (max_x != input.width) {
                    sum_type total = row0[max_x];
                    total += row1[max_x];
                    out[output.width - 1] = mean2(total);
                }
            }

            if (max_y != input.height) {
                const E* row = &input.data[input.offset + (input.height - 1)*input.stride];
                E* out = &output.data[output.offset + (output.height - 1)*output.stride];

                for (uint32_t x = 0; x < max_x; x += 2) {
                    sum_type total = row[x];
                    total += row[x + 1];
                    out[x/2] = mean2(total);
                }

                if (max_x != input.width)
                    out[output.width - 1] = row[max_x];
            }
        }

        /// Regions of any width, the same as BoofCV's ImplAverageDownSampleN
        template<class E>
        static void down_n( const Gray<E>& input, uint32_t width, Gray<E>& output ) {
            typedef typename TypeInfo<E>::sum_type sum_type;

            // number of pixels in the regions along the right and bottom borders
            const uint32_t rem_x = input.width%width;
            const uint32_t rem_y = input.height%width;
            const RegionMean<E> mean_inner(width*width), mean_right(width*rem_x);
            const RegionMean<E> mean_bottom(rem_y*width), mean_corner(rem_y*rem_x);

            for (uint32_t y = 0; y < input.height; y += width) {
                const uint32_t rows = std::min(width, input.height - y);
                const bool bottom = rows != width;
                E* out = &output.data[output.offset + (y/width)*output.stride];

                for (uint32_t x = 0; x < input.width; x += width) {
                    const uint32_t cols = std::min(width, input.width - x);

                    sum_type total = 0;
                    for (uint32_t yy = 0; yy < rows; yy++) {
                        const E* row = &input.data[input.offset + (y + yy)*input.stride + x];
                        for (uint32_t xx = 0; xx < cols; xx++) {
                            total += row[xx];
                        }
                    }

                    if (cols == width)
                        out[x/width] = bottom ? mean_bottom(total) : mean_inner(total);
                    else
                        out[x/width] = bottom ? mean_corner(total) : mean_right(total);
                }
            }
        }
    };
}

#endif
