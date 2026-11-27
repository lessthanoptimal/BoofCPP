#ifndef BOOFCPP_IMAGE_BLUR_H
#define BOOFCPP_IMAGE_BLUR_H

#include "boofcv/convolve.h"
#include "boofcv/image_border.h"
#include "boofcv/sanity_checks.h"
#include <algorithm>
#include <math.h>
#include <type_traits>
#include <vector>

namespace boofcv {
    /**
     * Functions for applying a mean convolution kernel across the image. Speeds up the operation by performing
     * a horizontal and vertical 1D convolution.
     */
    class ConvolveImageMean {
    public:
        /// Floating point running sums along y start over every VERTICAL_RESTART rows. BoofCV's
        /// ImplConvolveBox.VERTICAL_RESTART
        static constexpr int32_t VERTICAL_RESTART = 64;

        /// Rows of the integer mean along x computed at once
        static constexpr int32_t INTEGER_ROWS = 4;

        /// Rows of the floating point mean along x computed at once: as many as fit in 16 bytes
        template< class E>
        static constexpr int32_t FloatRows = (int32_t)(16/sizeof(E));

        /// Number of blocks of VERTICAL_RESTART inner rows for a kernel 'length' rows long. BoofCV's
        /// ImplConvolveBox.restartBlocks()
        static uint32_t restart_blocks( uint32_t height, uint32_t length ) {
            return (uint32_t)std::max<int64_t>(1, ((int64_t)height - length + VERTICAL_RESTART)/VERTICAL_RESTART);
        }

        /// First output row of the block, for a kernel that starts 'offset' rows before the output row. Block 0
        /// includes the top border and the last block the bottom border. BoofCV's ImplConvolveBox.restartRow()
        static uint32_t restart_row( uint32_t block, uint32_t blocks, uint32_t height, uint32_t offset ) {
            if (block == 0)
                return 0;
            if (block >= blocks)
                return height;
            return offset + block*VERTICAL_RESTART;
        }

        template< class E>
        static void inner_horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            integer_inner_horizontal(input, output, radius, radius*2 + 1, 0, input.height);
        }

        /// Integer mean along x of the inner image, rows y0 to y1. INTEGER_ROWS rows at a time, so their running
        /// sums don't wait on each other. The sums go to a buffer that's then divided, which vectorizes
        template< class E>
        static void integer_inner_horizontal( const Gray<E>& input, Gray<E>& output, uint32_t offset,
                                              uint32_t length_, uint32_t y0, uint32_t y1 ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            const auto length = (int32_t)length_, width = (int32_t)input.width;
            if (length > width)
                return;
            const MeanDivisor<E> divisor(length);
            if (!divisor.uses_multiply32()) {
                // the division doesn't vectorize, so it's done with the running sum
                for (uint32_t y = y0; y < y1; y++) {
                    const E* in = input.data + input.offset + y*input.stride;
                    E* out = output.data + output.offset + y*output.stride + offset;
                    sum_type total = 0;
                    for (int32_t i = 0; i < length; i++)
                        total += in[i];
                    out[0] = divisor(total);
                    for (int32_t i = length; i < width; i++) {
                        total += in[i] - in[i - length];
                        out[i - length + 1] = divisor(total);
                    }
                }
                return;
            }
            const int32_t n = width - length + 1;
            std::vector<sum_type> sums((size_t)INTEGER_ROWS*n);
            for (uint32_t y = y0; y < y1; y += INTEGER_ROWS) {
                const auto rows = (int32_t)std::min<uint32_t>(INTEGER_ROWS, y1 - y);
                const E* in[INTEGER_ROWS];
                sum_type* s[INTEGER_ROWS];
                sum_type total[INTEGER_ROWS];
                for (int32_t r = 0; r < INTEGER_ROWS; r++) {
                    // past the last row, the last row is computed again
                    in[r] = input.data + input.offset + (y + std::min(r, rows - 1))*input.stride;
                    s[r] = sums.data() + (size_t)r*n;
                    total[r] = 0;
                }
                for (int32_t i = 0; i < length; i++)
                    for (int32_t r = 0; r < INTEGER_ROWS; r++)
                        total[r] += in[r][i];
                for (int32_t r = 0; r < INTEGER_ROWS; r++)
                    s[r][0] = total[r];
                for (int32_t i = length; i < width; i++) {
                    for (int32_t r = 0; r < INTEGER_ROWS; r++) {
                        total[r] += in[r][i] - in[r][i - length];
                        s[r][i - length + 1] = total[r];
                    }
                }
                for (int32_t r = 0; r < rows; r++) {
                    E* out = output.data + output.offset + (y + r)*output.stride + offset;
                    for (int32_t i = 0; i < n; i++)
                        out[i] = divisor.multiply32(s[r][i]);
                }
            }
        }

        /// One row of a running sum along y: totals += added - removed, out = their mean. With the 32-bit
        /// multiplication the division is a second loop, which vectorizes
        template< class E>
        static void vertical_row( const MeanDivisor<E>& divisor, typename TypeInfo<E>::sum_type* totals,
                                  const E* removed, const E* added, E* out, int32_t n ) {
            if (divisor.uses_multiply32()) {
                for (int32_t i = 0; i < n; i++)
                    totals[i] = totals[i] - removed[i] + added[i];
                for (int32_t i = 0; i < n; i++)
                    out[i] = divisor.multiply32(totals[i]);
            } else {
                for (int32_t i = 0; i < n; i++) {
                    const auto total = totals[i] - removed[i] + added[i];
                    totals[i] = total;
                    out[i] = divisor(total);
                }
            }
        }

        template< class E>
        static void inner_vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            typedef typename TypeInfo<E>::sum_type sum_type;
            uint32_t kernelWidth = radius*2 + 1;

            uint32_t backStep = kernelWidth*input.stride;

            const MeanDivisor<E> divisor(kernelWidth);
            std::vector<sum_type> totals(input.width);

            for( uint32_t x = 0; x < input.width; x++ ) {
                uint32_t indexIn = input.offset + x;
                uint32_t indexOut = output.offset + output.stride*radius + x;

                sum_type total = 0;
                uint32_t indexEnd = indexIn + input.stride*kernelWidth;
                for( ; indexIn < indexEnd; indexIn += input.stride) {
                    total += input.data[indexIn];
                }
                totals[x] = total;
                output.data[indexOut] = divisor(total);
            }

            // change the order it is processed in to reduce cache misses
            for( uint32_t y = radius+1; y < output.height-radius; y++ ) {
                uint32_t indexIn = input.offset + (y+radius)*input.stride;
                uint32_t indexOut = output.offset + y*output.stride;

                E* ptr_front = &input.data[ indexIn - backStep ];
                E* ptr_back = &input.data[ indexIn ];
                E* ptr_out = &output.data[indexOut];

                vertical_row(divisor, totals.data(), ptr_front, ptr_back, ptr_out, input.width);
            }
        }

        template< class E>
        static void horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            checkSameShape(input,output);

            typedef typename TypeInfo<E>::signed_type signed_type;

            if constexpr (std::is_floating_point<E>::value) {
                float_horizontal<E>(input, output, radius, radius*2 + 1, nullptr, 0, input.height);
                return;
            }
            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            if( kernel.width > input.width ) {
                ConvolveNormalized::horizontal(kernel,input,output);
            } else {
                ConvolveNormalized_JustBorder::horizontal(kernel, input ,output );
                inner_horizontal(input, output, radius);
            }
        }

        template< class E>
        static void vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            checkSameShape(input,output);

            typedef typename TypeInfo<E>::signed_type signed_type;

            if constexpr (std::is_floating_point<E>::value) {
                float_vertical<E>(input, output, radius, radius*2 + 1, nullptr, 0, input.height);
                return;
            }
            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            if( kernel.width > input.height ) {
                ConvolveNormalized::vertical(kernel,input,output);
            } else {
                ConvolveNormalized_JustBorder::vertical(kernel, input ,output );
                inner_vertical(input, output, radius);
            }
        }

        /// Mean along x over 'length' pixels starting 'offset' pixels to the left, where pixels outside the image
        /// are read from 'border'. BoofCV's ConvolveImageMean.horizontal(input, output, offset, length, border)
        /// for U8, S16, U16, F32 and F64. Integer images round to the nearest value.
        template< class E>
        static void horizontal( const Gray<E>& input, Gray<E>& output, uint32_t offset, uint32_t length,
                                ImageBorder<E>& border ) {
            output.reshape(input.width, input.height);
            border.setImage(input);
            horizontal(input, output, offset, length, border, 0, input.height);
        }

        /// horizontal() of the rows y0 <= y < y1. 'output' has the input's shape and 'border' has the input image
        template< class E>
        static void horizontal( const Gray<E>& input, Gray<E>& output, uint32_t offset, uint32_t length,
                                const ImageBorder<E>& border, uint32_t y0, uint32_t y1 ) {
            if constexpr (std::is_floating_point<E>::value) {
                float_horizontal<E>(input, output, offset, length, &border, y0, y1);
                return;
            }
            typedef typename TypeInfo<E>::sum_type sum_type;
            // Signed, since pixels outside the image have negative coordinates
            const auto ofs = (int32_t)offset, len = (int32_t)length;
            const int32_t width = input.width;
            const int32_t border_right = len - ofs - 1;
            const MeanDivisor<E> divisor(len);

            for (int32_t y = y0; y < (int32_t)y1; y++) {
                E* out = output.data + output.offset + y*output.stride;

                // the border. In an image narrower than the kernel every column is a border column
                auto border_pixel = [&]( int32_t x ) {
                    out[x] = border_sum<E>(divisor, len, [&]( int32_t k ) { return border.get(x + k - ofs, y); });
                };
                for (int32_t x = 0; x < std::min(ofs, width); x++)
                    border_pixel(x);
                for (int32_t x = std::max(0, width - border_right); x < width; x++)
                    border_pixel(x);

            }
            integer_inner_horizontal(input, output, offset, length, y0, y1);
        }

        /// Mean along y over 'length' pixels starting 'offset' pixels above, where pixels outside the image
        /// are read from 'border'. BoofCV's ConvolveImageMean.vertical(input, output, offset, length, border,
        /// work) for U8, S16, U16, F32 and F64.
        ///
        /// @param totals Work space
        template< class E>
        static void vertical( const Gray<E>& input, Gray<E>& output, uint32_t offset, uint32_t length,
                              ImageBorder<E>& border, std::vector<typename TypeInfo<E>::sum_type>& totals ) {
            output.reshape(input.width, input.height);
            border.setImage(input);
            vertical(input, output, offset, length, border, totals, 0, input.height);
        }

        /// vertical() of the rows y0 <= y < y1. 'output' has the input's shape and 'border' has the input image. Floating
        /// point running sums start over every VERTICAL_RESTART rows, so a range that starts at one of those rows gives the
        /// same results
        template< class E>
        static void vertical( const Gray<E>& input, Gray<E>& output, uint32_t offset, uint32_t length,
                              const ImageBorder<E>& border, std::vector<typename TypeInfo<E>::sum_type>& totals,
                              uint32_t y0, uint32_t y1 ) {
            if constexpr (std::is_floating_point<E>::value) {
                float_vertical<E>(input, output, offset, length, &border, y0, y1);
                return;
            }
            typedef typename TypeInfo<E>::sum_type sum_type;
            // Signed, since pixels outside the image have negative coordinates
            const auto ofs = (int32_t)offset, len = (int32_t)length;
            const int32_t width = input.width;
            const int32_t height = input.height;
            const int32_t border_bottom = len - ofs - 1;
            const MeanDivisor<E> divisor(len);

            // the border rows, see horizontal()
            for (int32_t x = 0; x < width; x++) {
                auto border_pixel = [&]( int32_t y ) {
                    output.unsafe_at(x, y) =
                            border_sum<E>(divisor, len, [&]( int32_t k ) { return border.get(x, y + k - ofs); });
                };
                for (int32_t y = y0; y < std::min({ofs, height, (int32_t)y1}); y++)
                    border_pixel(y);
                for (int32_t y = std::max({0, height - border_bottom, (int32_t)y0}); y < (int32_t)y1; y++)
                    border_pixel(y);
            }

            // inner image with a running sum for each column. Its output rows are offset to offset + height - length
            if (len > height)
                return;
            const int32_t first = std::max(ofs, (int32_t)y0);
            const int32_t last = std::min(ofs + height - len, (int32_t)y1 - 1);
            if (first > last)
                return;
            totals.resize(width);
            const E* in = input.data + input.offset + (first - ofs)*input.stride;
            for (int32_t x = 0; x < width; x++)
                totals[x] = in[x];
            for (int32_t y = 1; y < len; y++) {
                const E* row = in + y*input.stride;
                for (int32_t x = 0; x < width; x++)
                    totals[x] += row[x];
            }
            E* out = output.data + output.offset + first*output.stride;
            for (int32_t x = 0; x < width; x++)
                out[x] = divisor(totals[x]);
            for (int32_t y = first - ofs; y < last - ofs; y++) {
                const E* removed = input.data + input.offset + y*input.stride;
                const E* added = removed + len*input.stride;
                out = output.data + output.offset + (ofs + y + 1)*output.stride;
                vertical_row(divisor, totals.data(), removed, added, out, width);
            }
        }

    private:
        /// Mean of 'length' integer pixels along the image border, where pixel(k) returns the k-th pixel. The division
        /// is rounded
        template< class E, class PixelFunc>
        static E border_sum( const MeanDivisor<E>& divisor, int32_t length, PixelFunc pixel ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            sum_type total = 0;
            for (int32_t k = 0; k < length; k++)
                total += pixel(k);
            return divisor(total);
        }

    public:
        /// Floating point mean along x of the rows y0 to y1. BoofCV's ImplConvolveMean.horizontal(). Without a border
        /// the mean near the image border is of the pixels inside the image
        template< class E>
        static void float_horizontal( const Gray<E>& input, Gray<E>& output, uint32_t offset_, uint32_t length_,
                                      const ImageBorder<E>* border, uint32_t y0, uint32_t y1 ) {
            const auto offset = (int32_t)offset_, length = (int32_t)length_;
            const int32_t width = input.width;
            const int32_t offsetR = length - offset - 1;

            for (int32_t y = y0; y < (int32_t)y1; y++) {
                const E* in = input.data + input.offset + y*input.stride;
                E* out = output.data + output.offset + y*output.stride;

                if (border == nullptr && length <= width) {
                    float_border_inside(in, out, offset, length, width);
                    continue;
                }
                auto pixel = [&]( int32_t i ) -> E {
                    if (i >= 0 && i < width)
                        return in[i];
                    return border == nullptr ? 0 : border->get(i, y);
                };
                // Running sums along the left and then the right border. Without a border only pixels inside the
                // image are counted
                for (int side = 0; side < 2; side++) {
                    const int32_t xStart = side == 0 ? 0 : std::max(std::min(offset, width), width - offsetR);
                    const int32_t xEnd = side == 0 ? std::min(offset, width) : width;
                    E total = 0;
                    int32_t count = 0;
                    for (int32_t x = xStart; x < xEnd; x++) {
                        if (x == xStart) {
                            for (int32_t i = x - offset; i < x - offset + length; i++) {
                                total += pixel(i);
                                count += i >= 0 && i < width ? 1 : 0;
                            }
                        } else {
                            const int32_t removed = x - offset - 1, added = x - offset + length - 1;
                            total -= pixel(removed);
                            total += pixel(added);
                            count += (added < width ? 1 : 0) - (removed >= 0 ? 1 : 0);
                        }
                        out[x] = total/(border == nullptr ? count : length);
                    }
                }
            }

            // inner image: total += added - removed, divided by the length
            if (length > width)
                return;
            const E divisor = (E)length;
            int32_t y = float_inner_rows(input, output, offset, length, y0, y1);
            for (; y < (int32_t)y1; y++) {
                const E* in = input.data + input.offset + y*input.stride;
                E* out = output.data + output.offset + y*output.stride + offset;
                E total = 0;
                for (int32_t i = 0; i < length; i++)
                    total += in[i];
                out[0] = total/divisor;
                for (int32_t i = length; i < width; i++) {
                    total += in[i] - in[i - length];
                    out[i - length + 1] = total/divisor;
                }
            }
        }

    private:
        /// float_horizontal()'s inner image for groups of FloatRows<E> rows. Returns the first row it didn't do.
        ///
        /// Each row's running sum depends on its previous pixel, so a single row can't be vectorized. Instead the
        /// rows of a group advance together: blocks of 4 columns are copied into arrays where the group's rows are
        /// adjacent, which the compiler computes with vector instructions. Every row's operations are in the same
        /// order as one row at a time
        template< class E>
        static int32_t float_inner_rows( const Gray<E>& input, Gray<E>& output, int32_t offset, int32_t length,
                                         int32_t y0, int32_t y1 ) {
            constexpr int32_t R = FloatRows<E>, K = 4;
            const int32_t width = input.width;
            const E divisor = (E)length;
            int32_t y = y0;
            for (; y + R <= y1; y += R) {
                const E* in[R];
                E* out[R];
                E total[R];
                for (int32_t r = 0; r < R; r++) {
                    in[r] = input.data + input.offset + (y + r)*input.stride;
                    out[r] = output.data + output.offset + (y + r)*output.stride + offset;
                    total[r] = 0;
                }
                for (int32_t i = 0; i < length; i++)
                    for (int32_t r = 0; r < R; r++)
                        total[r] += in[r][i];
                for (int32_t r = 0; r < R; r++)
                    out[r][0] = total[r]/divisor;
                int32_t i = length;
                for (; i + K <= width; i += K) {
                    // added[k][r] is pixel i + k of row r
                    E added[K][R], removed[K][R], mean[K][R];
                    for (int32_t r = 0; r < R; r++) {
                        for (int32_t k = 0; k < K; k++) {
                            added[k][r] = in[r][i + k];
                            removed[k][r] = in[r][i + k - length];
                        }
                    }
                    for (int32_t k = 0; k < K; k++) {
                        for (int32_t r = 0; r < R; r++) {
                            total[r] += added[k][r] - removed[k][r];
                            mean[k][r] = total[r]/divisor;
                        }
                    }
                    for (int32_t r = 0; r < R; r++)
                        for (int32_t k = 0; k < K; k++)
                            out[r][i + k - length + 1] = mean[k][r];
                }
                for (int32_t r = 0; r < R; r++) {
                    for (int32_t j = i; j < width; j++) {
                        total[r] += in[r][j] - in[r][j - length];
                        out[r][j - length + 1] = total[r]/divisor;
                    }
                }
            }
            return y;
        }

        /// Border of one row without an ImageBorder, when the kernel fits inside the image: the mean of the pixels
        /// inside the image, with a running sum along each side
        template< class E>
        static void float_border_inside( const E* in, E* out, int32_t offset, int32_t length, int32_t width ) {
            const int32_t offsetR = length - offset - 1;
            if (offset > 0) {
                E total = 0;
                int32_t count = length - offset;
                for (int32_t i = 0; i < count; i++)
                    total += in[i];
                out[0] = total/count;
                for (int32_t x = 1; x < offset; x++) {
                    total += in[x + offsetR];
                    count++;
                    out[x] = total/count;
                }
            }
            if (offsetR > 0) {
                const int32_t x0 = width - offsetR;
                E total = 0;
                int32_t count = offsetR + offset;
                for (int32_t i = x0 - offset; i < width; i++)
                    total += in[i];
                out[x0] = total/count;
                for (int32_t x = x0 + 1; x < width; x++) {
                    total -= in[x - offset - 1];
                    count--;
                    out[x] = total/count;
                }
            }
        }

    public:
        /// Floating point mean along y of the output rows y0 to y1. BoofCV's ImplConvolveMean.vertical(). The
        /// running sums start over every VERTICAL_RESTART rows after the first inner row
        template< class E>
        static void float_vertical( const Gray<E>& input, Gray<E>& output, uint32_t offset_, uint32_t length_,
                                    const ImageBorder<E>* border, uint32_t y0_, uint32_t y1_ ) {
            const auto offset = (int32_t)offset_, length = (int32_t)length_;
            const auto y0 = (int32_t)y0_, y1 = (int32_t)y1_;
            const int32_t width = input.width, height = input.height;
            const int32_t offsetR = length - offset - 1;
            std::vector<E> totals(width);

            // border rows
            for (int32_t y = y0; y < y1; y++) {
                if (y >= offset && y < height - offsetR) {
                    y = height - offsetR - 1;
                    continue;
                }
                std::fill(totals.begin(), totals.end(), 0);
                int32_t count = 0;
                for (int32_t k = 0; k < length; k++) {
                    const int32_t j = y - offset + k;
                    if (j >= 0 && j < height) {
                        const E* row = input.data + input.offset + j*input.stride;
                        for (int32_t i = 0; i < width; i++)
                            totals[i] += row[i];
                        count++;
                    } else if (border != nullptr) {
                        for (int32_t i = 0; i < width; i++)
                            totals[i] += border->get(i, j);
                    }
                }
                const int32_t divisor = border == nullptr ? count : length;
                E* out = output.data + output.offset + y*output.stride;
                for (int32_t i = 0; i < width; i++)
                    out[i] = totals[i]/divisor;
            }

            // inner rows, in blocks that start over the running sums
            const int32_t inner0 = std::max(y0, offset), inner1 = std::min(y1, height - offsetR);
            const E divisor = (E)length;
            for (int32_t start = inner0; start < inner1; ) {
                const int32_t end = std::min(offset + ((start - offset)/VERTICAL_RESTART + 1)*VERTICAL_RESTART, inner1);
                const E* first = input.data + input.offset + (start - offset)*input.stride;
                for (int32_t i = 0; i < width; i++)
                    totals[i] = first[i];
                for (int32_t k = 1; k < length; k++) {
                    const E* row = first + k*input.stride;
                    for (int32_t i = 0; i < width; i++)
                        totals[i] += row[i];
                }
                E* out = output.data + output.offset + start*output.stride;
                for (int32_t i = 0; i < width; i++)
                    out[i] = totals[i]/divisor;
                for (int32_t y = start + 1; y < end; y++) {
                    const E* removed = input.data + input.offset + (y - offset - 1)*input.stride;
                    const E* added = removed + length*input.stride;
                    out = output.data + output.offset + y*output.stride;
                    for (int32_t i = 0; i < width; i++) {
                        const E total = totals[i] - removed[i] + added[i];
                        totals[i] = total;
                        out[i] = total/divisor;
                    }
                }
                start = end;
            }
        }
    };

    class BlurImageOps {
    public:
        /**
         * Applies a mean box filter.
         *
         * @param input Input image.  Not modified.
         * @param output Storage for output image.
         * @param radius Radius of the box blur function.
         * @param storage Storage for intermediate results.
         * @return Output blurred image.
         */
        template<class E>
        static void mean(const Gray<E> &input, Gray<E> &output, uint32_t radius, Gray<E> &storage) {
            mean(input, output, radius, radius, storage);
        }

        /// Mean box filter with a different radius along x and y. BoofCV's BlurImageOps.mean() for U8, U16,
        /// F32 and F64. Near the image border the box only covers pixels inside the image.
        ///
        /// @param storage Work space
        template<class E>
        static void mean( const Gray<E>& input, Gray<E>& output, uint32_t radius_x, uint32_t radius_y,
                          Gray<E>& storage ) {
            if (radius_x == 0 || radius_y == 0)
                throw invalid_argument("Radius must be > 0");

            output.reshape(input.width, input.height);
            storage.reshape(input.width, input.height);

            ConvolveImageMean::horizontal(input, storage, radius_x);
            ConvolveImageMean::vertical(storage, output, radius_y);
        }

        /// Mean box filter where pixels outside the image are read from 'border', so every box has the same number
        /// of pixels. BoofCV's BlurImageOps.meanB() for U8, U16, F32 and F64.
        ///
        /// @param storage Work space
        /// @param totals Work space
        template< class E>
        static void mean( const Gray<E>& input, Gray<E>& output, uint32_t radius_x, uint32_t radius_y,
                          ImageBorder<E>& border, Gray<E>& storage,
                          std::vector<typename TypeInfo<E>::sum_type>& totals ) {
            if (radius_x == 0 || radius_y == 0)
                throw invalid_argument("Radius must be > 0");
            storage.reshape(input.width, input.height);
            ConvolveImageMean::horizontal(input, storage, radius_x, radius_x*2 + 1, border);
            ConvolveImageMean::vertical(storage, output, radius_y, radius_y*2 + 1, border, totals);
        }

        /**
         * Applies a Gaussian filter.
         *
         * @param input Input image.  Not modified.
         * @param output Storage for output image.
         * @param sigma Distribution's sigma. If <= 0 then sigma is determined from width
         * @param width Width of the kernel. If <= 0 then width is determined from sigma
         * @param storage Storage for intermediate results.
         * @return Output blurred image.
         */
        template<class E>
        static void gaussian(const Gray<E> &input, Gray<E> &output, double sigma, int32_t width, Gray<E> &storage) {
            gaussian(input, output, sigma, width, sigma, width, storage);
        }

        /// Gaussian blur with a different kernel along x and y. BoofCV's BlurImageOps.gaussian() for U8, U16,
        /// F32 and F64. A sigma <= 0 is computed from the width, and a width <= 0 from the sigma.
        ///
        /// @param storage Work space
        template<class E>
        static void gaussian( const Gray<E>& input, Gray<E>& output, double sigma_x, int32_t width_x,
                              double sigma_y, int32_t width_y, Gray<E>& storage ) {
            typedef typename TypeInfo<E>::signed_type signed_type;

            Kernel1D<signed_type> kernel_x = FactoryKernel::gaussian1D<signed_type>(sigma_x, width_x);
            Kernel1D<signed_type> kernel_y = FactoryKernel::gaussian1D<signed_type>(sigma_y, width_y);

            gaussian(input, output, kernel_x, kernel_y, storage);
        }

        /// Gaussian blur with kernels that have already been computed, e.g. by BoofCV's FactoryKernelGaussian
        ///
        /// @param storage Work space
        template<class E>
        static void gaussian( const Gray<E>& input, Gray<E>& output,
                              const Kernel1D<typename TypeInfo<E>::signed_type>& kernel_x,
                              const Kernel1D<typename TypeInfo<E>::signed_type>& kernel_y, Gray<E>& storage ) {
            output.reshape(input.width, input.height);
            storage.reshape(input.width, input.height);

            ConvolveNormalized::horizontal(kernel_x, input, storage);
            ConvolveNormalized::vertical(kernel_y, storage, output);
        }
    };
}

#endif