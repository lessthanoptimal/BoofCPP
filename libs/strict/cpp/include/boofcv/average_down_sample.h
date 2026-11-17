#ifndef BOOFCPP_AVERAGE_DOWN_SAMPLE_H
#define BOOFCPP_AVERAGE_DOWN_SAMPLE_H

#include "boofcv/convolve.h"
#include "boofcv/image_types.h"
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

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

    /// Down samples an image by averaging the pixels inside regions, the same as BoofCV's AverageDownSampleOps.
    class AverageDownSampleOps {
    public:
        /// Type of the image between the horizontal and vertical passes: F64 for F64 images and F32 otherwise
        template<class E>
        using middle_type = typename std::conditional<std::is_same<E, F64>::value, F64, F32>::type;

        /// Length of a side in the down sampled image
        static uint32_t down_sample_size( uint32_t length, uint32_t width ) {
            return length/width + (length%width != 0 ? 1 : 0);
        }

        /// Averages square regions which are 'width' pixels wide. The regions start at (0,0) and the ones along the
        /// right and bottom borders only include the pixels inside the image. The output is reshaped to the down
        /// sampled size.
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

        /// Down samples to the size of 'output' by averaging rectangular regions, which can have a non-integer size
        /// and partially cover pixels. If 'centered' the regions are shifted so the output's pixels are centered on
        /// the input's. Same as BoofCV's AverageDownSampleOps.down(input, centered, output, middle). 'middle' stores
        /// the horizontal pass and is reshaped
        template<class E>
        static void down( const Gray<E>& input, bool centered, Gray<E>& output, Gray<middle_type<E>>& middle ) {
            middle.reshape(output.width, input.height);
            horizontal(input, centered, middle);
            vertical(middle, centered, output);
        }

        /// Down samples along the x-axis only, the same as BoofCV's AverageDownSampleOps.horizontal(). 'output' has the
        /// same height and is no wider than 'input'
        template<class E>
        static void horizontal( const Gray<E>& input, bool centered, Gray<middle_type<E>>& output ) {
            typedef middle_type<E> F;
            if (output.width == 0 || input.width < output.width || input.height != output.height)
                throw std::invalid_argument("Output must have the same height and be no wider than the input");

            const float scale = input.width/(float)output.width;
            const float offset = centered ? -(scale - 1.0f)/2.0f : 0.0f;
            const uint32_t border0 = centered ? 1 : 0;
            const uint32_t border1 = upper_border(input.width, output.width, offset, scale);
            // BoofCV writes past the end of each row in this case
            if (border1 < border0)
                throw std::invalid_argument("Output is too narrow for centered regions");

            for (uint32_t y = 0; y < output.height; y++) {
                const E* row = &input.data[input.offset + y*input.stride];
                F* out = &output.data[output.offset + y*output.stride];
                auto pixel = [row]( int32_t i ) { return row[i]; };

                if (centered)
                    out[0] = border_mean<F>(pixel, input.width, offset, scale, 0);

                for (uint32_t x = border0; x < border1; x++) {
                    const float x0 = x*scale + offset;
                    const float x1 = (x + 1)*scale + offset;
                    const int32_t i0 = (int32_t)x0;
                    const int32_t i1 = (int32_t)x1;

                    // pixels at the ends are only partially inside the region
                    F sum = (i0 + 1 - x0)*row[i0];
                    for (int32_t i = i0 + 1; i < i1; i++) {
                        sum += row[i];
                    }
                    if (i1 < x1)
                        sum += (x1 - i1)*row[i1];

                    out[x] = sum/scale;
                }

                if (border1 != output.width)
                    out[border1] = border_mean<F>(pixel, input.width, offset, scale, border1);
            }
        }

        /// Down samples along the y-axis only, the same as BoofCV's AverageDownSampleOps.vertical(). The input is the
        /// output of horizontal(). 'output' has the same width and is no taller than 'input'. Integer outputs round to
        /// the nearest value
        template<class F, class E>
        static void vertical( const Gray<F>& input, bool centered, Gray<E>& output ) {
            static_assert(std::is_same<F, middle_type<E>>::value, "Input must be the output of horizontal()");
            if (output.height == 0 || input.height < output.height || input.width != output.width)
                throw std::invalid_argument("Output must have the same width and be no taller than the input");

            const float scale = input.height/(float)output.height;
            const float offset = centered ? -(scale - 1.0f)/2.0f : 0.0f;
            const uint32_t border0 = centered ? 1 : 0;
            const uint32_t border1 = upper_border(input.height, output.height, offset, scale);

            // integer outputs are summed here, floats are summed in the output
            std::vector<F> work(std::is_same<E, F>::value ? 0 : output.width);

            for (uint32_t y = 0; y < output.height; y++) {
                F* sums;
                if constexpr (std::is_same<E, F>::value)
                    sums = &output.data[output.offset + y*output.stride];
                else
                    sums = work.data();

                if (y < border0 || y >= border1) {
                    for (uint32_t x = 0; x < output.width; x++) {
                        const F* column = &input.data[input.offset + x];
                        auto pixel = [column, &input]( int32_t i ) { return column[i*input.stride]; };
                        sums[x] = border_mean<F>(pixel, input.height, offset, scale, y);
                    }
                } else {
                    const float y0 = y*scale + offset;
                    const float y1 = (y + 1)*scale + offset;
                    const int32_t i0 = (int32_t)y0;
                    const int32_t i1 = (int32_t)y1;

                    // rows at the ends are only partially inside the region
                    const float weight0 = 1 - (y0 - i0);
                    const F* row = &input.data[input.offset + i0*input.stride];
                    for (uint32_t x = 0; x < output.width; x++) {
                        sums[x] = weight0*row[x];
                    }
                    for (int32_t i = i0 + 1; i < i1; i++) {
                        row = &input.data[input.offset + i*input.stride];
                        for (uint32_t x = 0; x < output.width; x++) {
                            sums[x] += row[x];
                        }
                    }
                    if (i1 < y1) {
                        const float weight1 = std::fmod(y1, 1.0f);
                        row = &input.data[input.offset + i1*input.stride];
                        for (uint32_t x = 0; x < output.width; x++) {
                            sums[x] += weight1*row[x];
                        }
                    }
                    for (uint32_t x = 0; x < output.width; x++) {
                        sums[x] /= scale;
                    }
                }

                if constexpr (!std::is_same<E, F>::value) {
                    E* out = &output.data[output.offset + y*output.stride];
                    for (uint32_t x = 0; x < output.width; x++) {
                        out[x] = (E)(int32_t)(sums[x] + 0.5f);
                    }
                }
            }
        }

    private:
        /// Number of output pixels, starting from the first, whose regions are entirely inside the input. Same as
        /// BoofCV's ImplAverageDownSample.upperBorder()
        static uint32_t upper_border( uint32_t input_length, uint32_t output_length, float offset, float scale ) {
            return output_length*scale - offset > input_length ? output_length - 1 : output_length;
        }

        /// Mean of the region for output pixel 'index' when the region might extend outside the input, the same as
        /// BoofCV's naivePixelHorizontal() and naivePixelVertical(). 'pixel(i)' is the i-th of 'length' input pixels
        /// along the row or column. F is the type of the sum
        template<class F, class Pixel>
        static F border_mean( Pixel pixel, uint32_t length, float offset, float scale, uint32_t index ) {
            float x0 = index*scale + offset;
            const float x1 = x0 + scale;

            int32_t i0 = (int32_t)std::floor((double)x0);
            const int32_t i1 = (int32_t)std::min((double)length, std::ceil((double)x1));
            if (i0 < 0) {
                i0 = 0;
                x0 = 0.0f;
            }

            float area = 0.0f;
            F sum = 0;
            for (int32_t i = i0; i < i1; i++) {
                const float intersection = std::min(i + 1.0f, x1) - x0;
                area += intersection;
                sum += intersection*pixel(i);
                x0 = i + 1.0f;
            }
            return sum/area;
        }

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
