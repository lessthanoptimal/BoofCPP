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
        /// Floating point running sums along y start over every VERTICAL_RESTART rows. Same as BoofCV's
        /// ImplConvolveBox.VERTICAL_RESTART
        static constexpr int32_t VERTICAL_RESTART = 64;

        /// Number of blocks of VERTICAL_RESTART inner rows for a kernel 'length' rows long. Same as BoofCV's
        /// ImplConvolveBox.restartBlocks()
        static uint32_t restart_blocks( uint32_t height, uint32_t length ) {
            return (uint32_t)std::max<int64_t>(1, ((int64_t)height - length + VERTICAL_RESTART)/VERTICAL_RESTART);
        }

        /// First output row of the block, for a kernel that starts 'offset' rows before the output row. Block 0
        /// includes the top border and the last block the bottom border. Same as BoofCV's ImplConvolveBox.restartRow()
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
            typedef typename TypeInfo<E>::sum_type sum_type;
            uint32_t kernelWidth = radius*2 + 1;

            const MeanDivisor<E> divisor(kernelWidth);

            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t indexIn = input.offset + input.stride*y;
                uint32_t indexOut = output.offset + output.stride*y + radius;
                E* ptr_out = &output.data[indexOut];

                sum_type total = 0;

                uint32_t indexEnd = indexIn + kernelWidth;

                for( ; indexIn < indexEnd; indexIn++ ) {
                    total += input.data[indexIn];
                }
                *ptr_out++ = divisor(total);

                E* ptr_front = &input.data[ indexIn - kernelWidth ];
                E* ptr_back = &input.data[ indexIn ];
                uint32_t N = input.width - kernelWidth;

                for( uint32_t i = N; i; i-- ) {
                    total = (total - *ptr_front++) + *ptr_back++;

                    *ptr_out++ = divisor(total);
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

                for( uint32_t x = 0; x < input.width; x++) {
                    sum_type total = totals[ x ]  - *ptr_front++;
                    totals[ x ] = total += *ptr_back++;

                    *ptr_out++ = divisor(total);
                }
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
            if( kernel.width > input.height ) { // Java: length > input.height
                ConvolveNormalized::vertical(kernel,input,output);
            } else {
                ConvolveNormalized_JustBorder::vertical(kernel, input ,output );
                inner_vertical(input, output, radius);
            }
        }

        /// Mean along x over 'length' pixels starting 'offset' pixels to the left, where pixels outside the image
        /// are read from 'border'. Same as BoofCV's ConvolveImageMean.horizontal(input, output, offset, length,
        /// border) for U8, S16, U16, F32 and F64, including the order of the floating point operations. Integer
        /// images round to the nearest value.
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

                // inner image with a running sum
                if (len > width)
                    continue;
                const E* in = input.data + input.offset + y*input.stride;
                sum_type total = 0;
                for (int32_t i = 0; i < len; i++)
                    total += in[i];
                out[ofs] = divisor(total);
                for (int32_t i = len; i < width; i++) {
                    total -= in[i - len];
                    total += in[i];
                    out[ofs + i - len + 1] = divisor(total);
                }
            }
        }

        /// Mean along y over 'length' pixels starting 'offset' pixels above, where pixels outside the image
        /// are read from 'border'. Same as BoofCV's ConvolveImageMean.vertical(input, output, offset, length,
        /// border, work) for U8, S16, U16, F32 and F64, including the order of the floating point operations.
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
                for (int32_t x = 0; x < width; x++) {
                    sum_type total = totals[x] - removed[x];
                    total += added[x];
                    out[x] = divisor(total);
                    totals[x] = total;
                }
            }
        }

    private:
        /// Mean of 'length' integer pixels along the image border, where pixel(k) returns the k-th pixel. The division
        /// is rounded, same as BoofCV
        template< class E, class PixelFunc>
        static E border_sum( const MeanDivisor<E>& divisor, int32_t length, PixelFunc pixel ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            sum_type total = 0;
            for (int32_t k = 0; k < length; k++)
                total += pixel(k);
            return divisor(total);
        }

    public:
        /// Floating point mean along x of the rows y0 to y1. Port of BoofCV's ImplConvolveMean.horizontal(), with the
        /// same order of operations. Without a border the mean near the image border is of the pixels inside the image
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
            for (int32_t y = y0; y < (int32_t)y1; y++) {
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
        /// Floating point mean along y of the output rows y0 to y1. Port of BoofCV's ImplConvolveMean.vertical(), with
        /// the same order of operations. The running sums start over every VERTICAL_RESTART rows after the first
        /// inner row
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
                for (int32_t y = start; y < end; y++) {
                    if (y > start) {
                        const E* removed = input.data + input.offset + (y - offset - 1)*input.stride;
                        const E* added = removed + length*input.stride;
                        for (int32_t i = 0; i < width; i++) {
                            E total = totals[i] - removed[i];
                            totals[i] = total + added[i];
                        }
                    }
                    E* out = output.data + output.offset + y*output.stride;
                    for (int32_t i = 0; i < width; i++)
                        out[i] = totals[i]/divisor;
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

        /// Mean box filter with a different radius along x and y. Same as BoofCV's BlurImageOps.mean() for U8, U16,
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
        /// of pixels. Same as BoofCV's BlurImageOps.meanB() for U8, U16, F32 and F64.
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

        /// Gaussian blur with a different kernel along x and y. Same as BoofCV's BlurImageOps.gaussian() for U8, U16,
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