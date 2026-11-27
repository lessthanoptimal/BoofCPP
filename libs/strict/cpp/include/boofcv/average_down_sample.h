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

    /// Mean of the pixels in a region with 'count' pixels. Floats divide by 'count' and integers
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

    /// Down samples an image by averaging the pixels inside regions. BoofCV's AverageDownSampleOps.
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

            if (width == 2) {
                down2(input, output);
            } else if constexpr (std::is_integral<E>::value) {
                down_integer(input, width, output);
            } else {
                switch (width) {
                    case 3: return down_float<3>(input, output);
                    case 4: return down_float<4>(input, output);
                    case 5: return down_float<5>(input, output);
                    case 6: return down_float<6>(input, output);
                    case 7: return down_float<7>(input, output);
                    case 8: return down_float<8>(input, output);
                    default: return down_n(input, width, output);
                }
            }
        }

        /// Down samples to the size of 'output' by averaging rectangular regions, which can have a non-integer size
        /// and partially cover pixels. If 'centered' the regions are shifted so the output's pixels are centered on
        /// the input's. BoofCV's AverageDownSampleOps.down(input, centered, output, middle). 'middle' stores
        /// the horizontal pass and is reshaped
        template<class E>
        static void down( const Gray<E>& input, bool centered, Gray<E>& output, Gray<middle_type<E>>& middle ) {
            middle.reshape(output.width, input.height);
            horizontal(input, centered, middle);
            vertical(middle, centered, output);
        }

        /// Down samples along the x-axis only. BoofCV's AverageDownSampleOps.horizontal(). 'output' has the
        /// same height and is no wider than 'input'. Output pixel x averages the input from x0 to x1, and the pixels at
        /// the ends count by how much of them is inside:
        ///
        ///     x0 = x*scale + offset,  x1 = (x + 1)*scale + offset,  i0 = (int)x0,  i1 = (int)x1
        ///     sum = (i0 + 1 - x0)*in[i0] + in[i0 + 1] + ... + in[i1 - 1]  (added in this order)
        ///     if i1 < x1: sum += (x1 - i1)*in[i1]
        ///     out[x] = sum/scale
        ///
        /// The first output pixel if 'centered' and the last one if its region extends past the input are found by
        /// border_mean(). The regions are the same for every row, so they're found once, and 4 rows are summed at once
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

            const std::vector<Region> regions = inner_regions(border0, border1, offset, scale);
            uint32_t y = 0;
            for (; y + 4 <= output.height; y += 4)
                horizontal_rows<4, E, F>(input, output, y, regions.data(), centered, offset, scale);
            for (; y < output.height; y++)
                horizontal_rows<1, E, F>(input, output, y, regions.data(), centered, offset, scale);
        }

        /// Down samples along the y-axis only. BoofCV's AverageDownSampleOps.vertical(). The input is the
        /// output of horizontal(). 'output' has the same width and is no taller than 'input'. The same math as
        /// horizontal() along columns, except the last row's weight is fmod(y1, 1). Integer outputs round to the
        /// nearest value
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
        /// Number of output pixels, starting from the first, whose regions are entirely inside the input.
        /// BoofCV's ImplAverageDownSample.upperBorder()
        static uint32_t upper_border( uint32_t input_length, uint32_t output_length, float offset, float scale ) {
            return output_length*scale - offset > input_length ? output_length - 1 : output_length;
        }

        /// Mean of the region of output pixel 'index' when it might extend outside the input. BoofCV's
        /// naivePixelHorizontal() and naivePixelVertical(). 'pixel(i)' is the i-th of 'length' input pixels along the
        /// row or column, and F is the type of the sum. The region is clipped to the input, and each pixel is weighted
        /// by how much of it is inside:
        ///
        ///     x0 = index*scale + offset,  x1 = x0 + scale
        ///     for i from max(0, floor(x0)) to min(length, ceil(x1)) - 1, where x0 is 0 if it's below 0:
        ///         weight = min(i + 1, x1) - x0,  sum += weight*pixel(i),  area += weight,  x0 = i + 1
        ///     mean = sum/area
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

        /// Mean of 'COUNT' pixels, the same as RegionMean. With the count known at compile time, the compiler can
        /// replace an integer division
        template<class E, uint32_t COUNT>
        static E constant_mean( typename TypeInfo<E>::sum_type total ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            if constexpr (std::is_floating_point<E>::value)
                return static_cast<E>(total/(sum_type)COUNT);
            else if constexpr (std::is_signed<E>::value)
                return static_cast<E>(divide_round_away(total, (sum_type)COUNT, (sum_type)(COUNT/2)));
            else
                return static_cast<E>((total + COUNT/2)/COUNT);
        }

        /// Regions which are 2 pixels wide. BoofCV's ImplAverageDownSample2:
        ///
        ///     out[y][x] = mean(in[2y][2x] + in[2y][2x + 1] + in[2y + 1][2x] + in[2y + 1][2x + 1])
        ///
        /// The pixels are added in this order. Regions along the right and bottom borders have 2 pixels, and the
        /// corner 1
        template<class E>
        static void down2( const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            const uint32_t pairs = input.width/2;
            const uint32_t max_y = input.height - input.height%2;

            for (uint32_t y = 0; y < max_y; y += 2) {
                const E* row0 = &input.data[input.offset + y*input.stride];
                const E* row1 = row0 + input.stride;
                E* out = &output.data[output.offset + (y/2)*output.stride];
                for (uint32_t i = 0; i < pairs; i++) {
                    sum_type total = row0[2*i];
                    total += row0[2*i + 1];
                    total += row1[2*i];
                    total += row1[2*i + 1];
                    out[i] = constant_mean<E, 4>(total);
                }
                if (input.width%2 != 0) {
                    sum_type total = row0[2*pairs];
                    total += row1[2*pairs];
                    out[pairs] = constant_mean<E, 2>(total);
                }
            }

            if (max_y != input.height) {
                const E* row = &input.data[input.offset + (input.height - 1)*input.stride];
                E* out = &output.data[output.offset + (output.height - 1)*output.stride];
                for (uint32_t i = 0; i < pairs; i++) {
                    sum_type total = row[2*i];
                    total += row[2*i + 1];
                    out[i] = constant_mean<E, 2>(total);
                }
                if (input.width%2 != 0)
                    out[pairs] = row[2*pairs];
            }
        }

        /// Integer regions of any width. BoofCV's ImplAverageDownSampleN. Integer sums don't depend on the
        /// order, so each band of 'width' rows is summed in two steps:
        ///
        ///     column[x] = in[y][x] + in[y + 1][x] + ... + in[y + width - 1][x]       for every x
        ///     out[i] = mean(column[i*width] + ... + column[i*width + width - 1])
        ///
        /// The first step is the same for every x, so the compiler computes several at once. The sums are unsigned so
        /// that S32 wraps. Regions along the right and bottom borders have fewer pixels
        template<class E>
        static void down_integer( const Gray<E>& input, uint32_t width, Gray<E>& output ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            typedef std::make_unsigned_t<sum_type> total_type;
            const uint32_t rem_x = input.width%width;
            const uint32_t rem_y = input.height%width;
            const uint32_t regions_x = input.width/width;
            const RegionMean<E> mean_inner(width*width), mean_right(width*rem_x);
            const RegionMean<E> mean_bottom(rem_y*width), mean_corner(rem_y*rem_x);
            // a local copy lets the compiler vectorize the loops
            const uint32_t w = input.width;
            std::vector<total_type> columns(w);
            total_type* column = columns.data();

            for (uint32_t y = 0; y < input.height; y += width) {
                const uint32_t rows = std::min(width, input.height - y);
                const bool bottom = rows != width;
                E* out = &output.data[output.offset + (y/width)*output.stride];

                const E* row = &input.data[input.offset + y*input.stride];
                for (uint32_t x = 0; x < w; x++)
                    column[x] = (total_type)(sum_type)row[x];
                for (uint32_t yy = 1; yy < rows; yy++) {
                    row = &input.data[input.offset + (y + yy)*input.stride];
                    for (uint32_t x = 0; x < w; x++)
                        column[x] += (total_type)(sum_type)row[x];
                }

                const RegionMean<E>& mean = bottom ? mean_bottom : mean_inner;
                for (uint32_t i = 0; i < regions_x; i++) {
                    total_type total = 0;
                    for (uint32_t xx = 0; xx < width; xx++)
                        total += column[i*width + xx];
                    out[i] = mean((sum_type)total);
                }
                if (rem_x != 0) {
                    total_type total = 0;
                    for (uint32_t xx = 0; xx < rem_x; xx++)
                        total += column[regions_x*width + xx];
                    out[regions_x] = bottom ? mean_corner((sum_type)total) : mean_right((sum_type)total);
                }
            }
        }

        /// Float regions W pixels wide. BoofCV's ImplAverageDownSampleN. Float sums depend on the order, so
        /// each region is added in this order:
        ///
        ///     total = 0
        ///     for r in 0..W-1: for c in 0..W-1: total += in[y + r][x + c]
        ///     out = mean(total)
        ///
        /// With W known at compile time the compiler sums several regions at once. Up to W = 5 each region is summed
        /// in turn. Wider regions and those along the bottom keep a total for every region and add a row at a time,
        /// which is faster for them. Regions along the right and bottom borders have fewer pixels
        template<uint32_t W, class E>
        static void down_float( const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            const uint32_t rem_x = input.width%W;
            const uint32_t rem_y = input.height%W;
            const uint32_t regions_x = input.width/W;
            const RegionMean<E> mean_inner(W*W), mean_right(W*rem_x);
            const RegionMean<E> mean_bottom(rem_y*W), mean_corner(rem_y*rem_x);
            std::vector<sum_type> totals(regions_x);

            for (uint32_t y = 0; y < input.height; y += W) {
                const uint32_t rows = std::min(W, input.height - y);
                const bool bottom = rows != W;
                const RegionMean<E>& mean = bottom ? mean_bottom : mean_inner;
                E* out = &output.data[output.offset + (y/W)*output.stride];

                if (W <= 5 && !bottom) {
                    const E* row[W];
                    for (uint32_t r = 0; r < W; r++)
                        row[r] = &input.data[input.offset + (y + r)*input.stride];
                    for (uint32_t i = 0; i < regions_x; i++) {
                        sum_type total = 0;
                        for (uint32_t r = 0; r < W; r++)
                            for (uint32_t xx = 0; xx < W; xx++)
                                total += row[r][i*W + xx];
                        out[i] = mean(total);
                    }
                } else {
                    std::fill(totals.begin(), totals.end(), (sum_type)0);
                    for (uint32_t r = 0; r < rows; r++) {
                        const E* row = &input.data[input.offset + (y + r)*input.stride];
                        for (uint32_t i = 0; i < regions_x; i++) {
                            sum_type total = totals[i];
                            for (uint32_t xx = 0; xx < W; xx++)
                                total += row[i*W + xx];
                            totals[i] = total;
                        }
                    }
                    for (uint32_t i = 0; i < regions_x; i++)
                        out[i] = mean(totals[i]);
                }

                if (rem_x != 0) {
                    sum_type total = 0;
                    for (uint32_t r = 0; r < rows; r++) {
                        const E* row = &input.data[input.offset + (y + r)*input.stride + regions_x*W];
                        for (uint32_t xx = 0; xx < rem_x; xx++)
                            total += row[xx];
                    }
                    out[regions_x] = bottom ? mean_corner(total) : mean_right(total);
                }
            }
        }

        /// Float regions of width 1 and wider than 8. The same order as down_float(), one region at a time
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

        /// Region of an output pixel which is inside the input: the first pixel i0 with weight w0, the pixels up to
        /// i1, and i1 with weight w1 if 'partial'. See horizontal()
        struct Region {
            int32_t i0, i1;
            float w0, w1;
            bool partial;
        };

        static std::vector<Region> inner_regions( uint32_t border0, uint32_t border1, float offset, float scale ) {
            std::vector<Region> regions(border1);
            for (uint32_t x = border0; x < border1; x++) {
                const float x0 = x*scale + offset;
                const float x1 = (x + 1)*scale + offset;
                Region& region = regions[x];
                region.i0 = (int32_t)x0;
                region.i1 = (int32_t)x1;
                region.w0 = region.i0 + 1 - x0;
                region.partial = region.i1 < x1;
                region.w1 = x1 - region.i1;
            }
            return regions;
        }

        /// Output rows y to y + R - 1 of horizontal(). The rows' sums don't depend on each other, so they're computed
        /// at the same time
        template<uint32_t R, class E, class F>
        static void horizontal_rows( const Gray<E>& input, Gray<F>& output, uint32_t y, const Region* regions,
                                     bool centered, float offset, float scale ) {
            const uint32_t border0 = centered ? 1 : 0;
            const uint32_t border1 = upper_border(input.width, output.width, offset, scale);
            const E* rows[R];
            F* outs[R];
            for (uint32_t r = 0; r < R; r++) {
                rows[r] = &input.data[input.offset + (y + r)*input.stride];
                outs[r] = &output.data[output.offset + (y + r)*output.stride];
            }

            for (uint32_t x = border0; x < border1; x++) {
                const Region region = regions[x];
                F sum[R];
                for (uint32_t r = 0; r < R; r++)
                    sum[r] = region.w0*rows[r][region.i0];
                for (int32_t i = region.i0 + 1; i < region.i1; i++)
                    for (uint32_t r = 0; r < R; r++)
                        sum[r] += rows[r][i];
                if (region.partial)
                    for (uint32_t r = 0; r < R; r++)
                        sum[r] += region.w1*rows[r][region.i1];
                for (uint32_t r = 0; r < R; r++)
                    outs[r][x] = sum[r]/scale;
            }

            for (uint32_t r = 0; r < R; r++) {
                const E* row = rows[r];
                auto pixel = [row]( int32_t i ) { return row[i]; };
                if (centered)
                    outs[r][0] = border_mean<F>(pixel, input.width, offset, scale, 0);
                if (border1 != output.width)
                    outs[r][border1] = border_mean<F>(pixel, input.width, offset, scale, border1);
            }
        }
    };
}

#endif
