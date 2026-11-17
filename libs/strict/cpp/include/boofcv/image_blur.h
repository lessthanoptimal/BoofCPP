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

        /**
         * Image border for mean filter along x-axis. Floating point images. Port of BoofCV
         * ImplConvolveMean.horizontalBorder(GrayF32) so that the order of floating point operations is identical:
         * running sum of pixels inside the image divided by the number of pixels.
         */
        template< class E>
        static void border_horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            const int32_t offset = radius;
            const int32_t length = radius*2 + 1;
            const int32_t offsetR = length - offset - 1;
            const int32_t width = input.width;
            const E* dataSrc = input.data;
            E* dataDst = output.data;

            for( uint32_t y = 0; y < input.height; y++ ) {
                int32_t indexDest = output.offset + y*output.stride;
                int32_t j = input.offset + y*input.stride;

                E total = 0;
                int32_t count = length - offset;
                int32_t jEnd = j + count;
                if( offset > 0 ) {
                    for( int32_t indexSrc = j; indexSrc < jEnd; indexSrc++ ) {
                        total += dataSrc[indexSrc];
                    }
                    dataDst[indexDest++] = total/count;
                }
                while( ++count < length ) {
                    total += dataSrc[jEnd++];
                    dataDst[indexDest++] = total/count;
                }

                jEnd = j + width;
                count = offset + offsetR;
                j += width - count;
                indexDest += width - count;
                total = 0;
                if( offsetR > 0 ) {
                    for( int32_t indexSrc = j; indexSrc < jEnd; indexSrc++ ) {
                        total += dataSrc[indexSrc];
                    }
                    dataDst[indexDest++] = total/count;
                }
                while( --count > offset ) {
                    total -= dataSrc[j++];
                    dataDst[indexDest++] = total/count;
                }
            }
        }

        /**
         * Image border for mean filter along y-axis. Floating point images. Port of BoofCV
         * ImplConvolveMean.verticalBorder(GrayF32) so that the order of floating point operations is identical.
         */
        template< class E>
        static void border_vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            const int32_t offset = radius;
            const int32_t length = radius*2 + 1;
            const int32_t height = input.height;
            const int32_t x0 = 0, x1 = input.width;
            const E* dataSrc = input.data;
            E* dataDst = output.data;
            std::vector<E> totals(x1 - x0);

            // Image Top
            for( int32_t count = length - offset; count < length; count++ ) {
                const int32_t indexInRow = input.offset + x0;
                if( count == length - offset ) {
                    int32_t indexIn = indexInRow;
                    for( int32_t x = x0; x < x1; x++ ) {
                        totals[x - x0] = dataSrc[indexIn++];
                    }
                    for( int32_t y = 1; y < count; y++ ) {
                        indexIn = indexInRow + y*input.stride;
                        for( int32_t x = x0; x < x1; x++ ) {
                            totals[x - x0] += dataSrc[indexIn++];
                        }
                    }
                } else {
                    int32_t indexIn0 = indexInRow + (count - 1)*input.stride;
                    int32_t end = indexIn0 + x1 - x0;
                    for( int32_t i = indexIn0; i < end; i++ ) {
                        totals[i - indexIn0] += dataSrc[i];
                    }
                }
                int32_t indexOut = output.offset + (count - (length - offset))*output.stride;
                for( int32_t x = x0; x < x1; x++ ) {
                    dataDst[indexOut + x] = totals[x - x0]/count;
                }
            }
            // Image Bottom
            for( int32_t yStart = height - length + 1; yStart < height - offset; yStart++ ) {
                const int32_t indexInRow = input.offset + x0;
                if( yStart == height - length + 1 ) {
                    int32_t indexIn = indexInRow + yStart*input.stride;
                    for( int32_t x = x0; x < x1; x++ ) {
                        totals[x - x0] = dataSrc[indexIn++];
                    }
                    for( int32_t y = yStart + 1; y < height; y++ ) {
                        indexIn = indexInRow + y*input.stride;
                        for( int32_t x = x0; x < x1; x++ ) {
                            totals[x - x0] += dataSrc[indexIn++];
                        }
                    }
                } else {
                    int32_t indexIn0 = indexInRow + (yStart - 1)*input.stride;
                    int32_t indexIn1 = indexIn0 + x1 - x0;
                    for( int32_t i = indexIn0; i < indexIn1; i++ ) {
                        totals[i - indexIn0] -= dataSrc[i];
                    }
                }
                int32_t count = height - yStart;
                int32_t indexOut = output.offset + (yStart + offset)*output.stride;
                for( int32_t x = x0; x < x1; x++ ) {
                    dataDst[indexOut + x] = totals[x - x0]/count;
                }
            }
        }

        template< class E>
        static void horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            checkSameShape(input,output);

            typedef typename TypeInfo<E>::signed_type signed_type;

            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            if( kernel.width > input.width ) {
                ConvolveNormalized::horizontal(kernel,input,output);
            } else {
                if constexpr (std::is_floating_point<E>::value)
                    border_horizontal(input, output, radius);
                else
                    ConvolveNormalized_JustBorder::horizontal(kernel, input ,output );
                inner_horizontal(input, output, radius);
            }
        }

        template< class E>
        static void vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius )
        {
            checkSameShape(input,output);

            typedef typename TypeInfo<E>::signed_type signed_type;

            Kernel1D<signed_type> kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
            if( kernel.width > input.height ) { // Java: length > input.height
                ConvolveNormalized::vertical(kernel,input,output);
            } else {
                if constexpr (std::is_floating_point<E>::value)
                    border_vertical(input, output, radius);
                else
                    ConvolveNormalized_JustBorder::vertical(kernel, input ,output );
                inner_vertical(input, output, radius);
            }
        }

        /// Mean along x over 'length' pixels starting 'offset' pixels to the left, where pixels outside the image
        /// are read from 'border'. Same as BoofCV's ConvolveImageMean.horizontal(input, output, offset, length,
        /// border) for U8, S16, U16, F32 and F64, including the order of the floating point operations. Integer
        /// images round to the nearest value.
        template< class E>
        static void horizontal( const Gray<E>& input, Gray<E>& output, int32_t offset, int32_t length,
                                ImageBorder<E>& border ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            output.reshape(input.width, input.height);
            border.setImage(input);
            const int32_t width = input.width;
            const int32_t height = input.height;
            const int32_t border_right = length - offset - 1;
            const MeanDivisor<E> divisor(length);

            for (int32_t y = 0; y < height; y++) {
                E* out = output.data + output.offset + y*output.stride;

                // the border. BoofCV fails if the image is narrower than offset or border_right, so those columns
                // are clamped to the image
                auto border_pixel = [&]( int32_t x ) {
                    out[x] = border_sum<E>(divisor, length, [&]( int32_t k ) { return border.get(x + k - offset, y); });
                };
                for (int32_t x = 0; x < std::min(offset, width); x++)
                    border_pixel(x);
                for (int32_t x = std::max(0, width - border_right); x < width; x++)
                    border_pixel(x);

                // inner image with a running sum
                if (length > width)
                    continue;
                const E* in = input.data + input.offset + y*input.stride;
                sum_type total = 0;
                for (int32_t i = 0; i < length; i++)
                    total += in[i];
                out[offset] = divisor(total);
                for (int32_t i = length; i < width; i++) {
                    total -= in[i - length];
                    total += in[i];
                    out[offset + i - length + 1] = divisor(total);
                }
            }
        }

        /// Mean along y over 'length' pixels starting 'offset' pixels above, where pixels outside the image
        /// are read from 'border'. Same as BoofCV's ConvolveImageMean.vertical(input, output, offset, length,
        /// border, work) for U8, S16, U16, F32 and F64, including the order of the floating point operations.
        ///
        /// @param totals Work space
        template< class E>
        static void vertical( const Gray<E>& input, Gray<E>& output, int32_t offset, int32_t length,
                              ImageBorder<E>& border, std::vector<typename TypeInfo<E>::sum_type>& totals ) {
            typedef typename TypeInfo<E>::sum_type sum_type;
            output.reshape(input.width, input.height);
            border.setImage(input);
            const int32_t width = input.width;
            const int32_t height = input.height;
            const int32_t border_bottom = length - offset - 1;
            const MeanDivisor<E> divisor(length);

            // the border. Rows are clamped to the image, see horizontal()
            for (int32_t x = 0; x < width; x++) {
                auto border_pixel = [&]( int32_t y ) {
                    output.unsafe_at(x, y) =
                            border_sum<E>(divisor, length, [&]( int32_t k ) { return border.get(x, y + k - offset); });
                };
                for (int32_t y = 0; y < std::min(offset, height); y++)
                    border_pixel(y);
                for (int32_t y = std::max(0, height - border_bottom); y < height; y++)
                    border_pixel(y);
            }

            // inner image with a running sum for each column
            if (length > height)
                return;
            totals.resize(width);
            const E* in = input.data + input.offset;
            for (int32_t x = 0; x < width; x++)
                totals[x] = in[x];
            for (int32_t y = 1; y < length; y++) {
                const E* row = in + y*input.stride;
                for (int32_t x = 0; x < width; x++)
                    totals[x] += row[x];
            }
            E* out = output.data + output.offset + offset*output.stride;
            for (int32_t x = 0; x < width; x++)
                out[x] = divisor(totals[x]);
            for (int32_t y = 0; y < height - length; y++) {
                const E* removed = in + y*input.stride;
                const E* added = removed + length*input.stride;
                out = output.data + output.offset + (offset + y + 1)*output.stride;
                for (int32_t x = 0; x < width; x++) {
                    sum_type total = totals[x] - removed[x];
                    total += added[x];
                    out[x] = divisor(total);
                    totals[x] = total;
                }
            }
        }

    private:
        /// Mean of 'length' pixels along the image border, where pixel(k) returns the k-th pixel. Same as BoofCV's
        /// ConvolveJustBorder_General_SB with a mean kernel: floats add up each pixel times 1/length, and integers
        /// add up the pixels and round the division.
        template< class E, class PixelFunc>
        static E border_sum( const MeanDivisor<E>& divisor, int32_t length, PixelFunc pixel ) {
            if constexpr (std::is_floating_point<E>::value) {
                const E weight = (E)1/(E)length;
                E total = 0;
                for (int32_t k = 0; k < length; k++)
                    total += pixel(k)*weight;
                return total;
            } else {
                typedef typename TypeInfo<E>::sum_type sum_type;
                sum_type total = 0;
                for (int32_t k = 0; k < length; k++)
                    total += pixel(k);
                return divisor(total);
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
            if (radius_x <= 0 || radius_y <= 0)
                throw invalid_argument("Radius must be > 0");

            output.reshape(input.width, input.height);
            storage.reshape(input.width, input.height);

            ConvolveImageMean::horizontal(input, storage, radius_x);
            ConvolveImageMean::vertical(storage, output, radius_y);
        }

        /// Mean box filter where pixels outside the image are read from 'border'. Same as BoofCV's
        /// BlurImageOps.meanB() for U8, U16, F32 and F64, which fails if a radius is larger than the image. Here
        /// those images work.
        ///
        /// @param storage Work space
        /// @param totals Work space
        template< class E>
        static void mean_border( const Gray<E>& input, Gray<E>& output, int32_t radius_x, int32_t radius_y,
                                 ImageBorder<E>& border, Gray<E>& storage,
                                 std::vector<typename TypeInfo<E>::sum_type>& totals ) {
            if (radius_x <= 0 || radius_y <= 0)
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