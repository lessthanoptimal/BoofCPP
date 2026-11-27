#ifndef BOOFCPP_CONVOLVE_H
#define BOOFCPP_CONVOLVE_H

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <random>
#include <type_traits>

#include "boofcv/base_types.h"
#include "boofcv/image_types.h"
#include "boofcv/sanity_checks.h"
#include "boofcv/image_border.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/convolve_naive.h"
#include "boofcv/mean_divisor.h"

namespace boofcv {
    /// Number of pixels inside an image 'length' pixels long in the window [i - radius, i + radius]
    inline uint32_t window_count( uint32_t i, uint32_t radius, uint32_t length ) {
        uint32_t lower = i > radius ? i - radius : 0;
        uint32_t upper = std::min(i + radius, length - 1);
        return upper - lower + 1;
    }

    /// The image border of a normalized convolution. Each border pixel is the sum of the kernel weights inside the
    /// image times their pixels, divided by the sum of those weights. BoofCV's
    /// ConvolveNormalized_JustBorder_SB. The order of the sums, arranged so the rows vectorize:
    ///
    ///     horizontal:  for each border column:  weight = the previous column's, updated by one kernel element
    ///                      for each row:  out = sum of the kernel elements inside times their pixels / weight
    ///     vertical:    for each border row:  weight of the kernel elements inside
    ///                      sum_row(): the row's pixels in blocks, as in ConvolveImage_Inner
    ///     convolve:    for each left and right border column, with the corners:  one weight away from the corners
    ///                      for each row:  out = the sum over the kernel window inside / weight
    ///                  for each top and bottom border row:  one weight, the row by sum_row()
    class ConvolveNormalized_JustBorder {
    public:
        /// Left and right borders. The weight is updated incrementally from column to column. For integers it's the
        /// same as summing the weights inside
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::signed_type K;
            const int32_t kw = kernel.width;
            const int32_t offsetL = kernel.offset;
            const int32_t offsetR = kw - offsetL - 1;
            const int32_t width = input.width;
            const K* ker = kernel.data.data;

            // pixel x sums kernel elements k0 to k1 - 1, which start at input column x - offsetL + k0
            auto column = [&]( int32_t x, int32_t k0, int32_t k1, K weight ) {
                for (uint32_t y = 0; y < input.height; y++) {
                    const E* src = &input.data[input.offset + y*input.stride] + (x - offsetL);
                    K total = 0;
                    for (int32_t k = k0; k < k1; k++)
                        total += src[k]*ker[k];
                    output.data[output.offset + y*output.stride + x] = normalize<E>(total, weight);
                }
            };

            K weight = 0;
            for (int32_t k = kw - offsetR; k < kw; k++)
                weight += ker[k];
            for (int32_t x = 0; x < offsetL; x++) {
                weight += ker[offsetL - x];
                column(x, offsetL - x, kw, weight);
            }
            weight = 0;
            for (int32_t k = 0; k < kw; k++)
                weight += ker[k];
            for (int32_t r = 0; r < offsetR; r++) {
                weight -= ker[kw - 1 - r];
                column(width - offsetR + r, 0, kw - 1 - r, weight);
            }
        }

        /// Top and bottom borders
        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ) {
            vertical(kernel, input, output, 0, input.height);
        }

        /// The top and bottom border rows y0 <= y < y1
        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output, uint32_t y0, uint32_t y1 ) {
            typedef typename TypeInfo<E>::signed_type K;
            const uint32_t offsetL = kernel.offset;
            const uint32_t offsetR = kernel.width - offsetL - 1;

            // row y sums kernel elements k0 to k1 - 1, which start at input row y - offsetL + k0
            auto border_row = [&]( uint32_t y, uint32_t k0, uint32_t k1 ) {
                K weight = 0;
                for (uint32_t k = k0; k < k1; k++)
                    weight += kernel.data[k];
                sum_row(&output.data[output.offset + y*output.stride], input.width, weight, [&]( auto add ) {
                    for (uint32_t k = k0; k < k1; k++)
                        add(&input.data[input.offset + (y + k - offsetL)*input.stride], kernel.data[k]);
                });
            };
            for (uint32_t y = y0; y < std::min(y1, offsetL); y++)
                border_row(y, offsetL - y, kernel.width);
            for (uint32_t y = std::max(y0, input.height - offsetR); y < y1; y++)
                border_row(y, 0, input.height - (y - offsetL));
        }

        /// The border of a 2D kernel, with sums over kernel rows then columns
        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ) {
            convolve(kernel, input, output, 0, input.height);
        }

        /// The border pixels in rows y0 <= y < y1
        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output, uint32_t y0, uint32_t y1 ) {
            typedef typename TypeInfo<E>::signed_type K;
            const int32_t kw = kernel.width;
            const int32_t offsetL = kernel.offset;
            const int32_t offsetR = kw - offsetL - 1;
            const int32_t width = input.width;
            const int32_t height = input.height;

            // sum of the kernel weights in rows minI to maxI and columns minJ to maxJ, relative to its center
            auto window_weight = [&]( int32_t minI, int32_t maxI, int32_t minJ, int32_t maxJ ) {
                K weight = 0;
                for (int32_t i = minI; i <= maxI; i++) {
                    const K* ker = kernel.data.data + (i + offsetL)*kw + offsetL;
                    for (int32_t j = minJ; j <= maxJ; j++)
                        weight += ker[j];
                }
                check_weight(weight);
                return weight;
            };

            // one pixel. Its weight is summed with the total, unless 'given'
            auto pixel = [&]( int32_t x, int32_t y, int32_t minI, int32_t maxI, int32_t minJ, int32_t maxJ,
                              const K* given ) {
                K total = 0, weight = 0;
                for (int32_t i = minI; i <= maxI; i++) {
                    const E* src = input.data + input.offset + (y + i)*input.stride + x;
                    const K* ker = kernel.data.data + (i + offsetL)*kw + offsetL;
                    for (int32_t j = minJ; j <= maxJ; j++) {
                        const K w = ker[j];
                        if (!given)
                            weight += w;
                        total += src[j]*w;
                    }
                }
                if (given)
                    weight = *given;
                else
                    check_weight(weight);
                output.unsafe_at(x, y) = normalize<E>(total, weight);
            };

            // left and right borders, with the corners. Rows away from the top and bottom share the column's weight
            auto column = [&]( int32_t x, int32_t minJ, int32_t maxJ ) {
                const K weight = window_weight(-offsetL, offsetR, minJ, maxJ);
                for (int32_t y = (int32_t)y0; y < (int32_t)y1; y++) {
                    const int32_t minI = y >= offsetL ? -offsetL : -y;
                    const int32_t maxI = y < height - offsetR ? offsetR : height - y - 1;
                    const bool inner = minI == -offsetL && maxI == offsetR;
                    pixel(x, y, minI, maxI, minJ, maxJ, inner ? &weight : nullptr);
                }
            };
            for (int32_t x = 0; x < offsetL; x++)
                column(x, -x, offsetR);
            for (int32_t x = width - offsetR; x < width; x++)
                column(x, -offsetL, width - x - 1);

            // top and bottom borders without the corners. Every pixel in a row has the same weight
            auto border_row = [&]( int32_t y, int32_t minI, int32_t maxI ) {
                sum_row(&output.data[output.offset + y*output.stride + offsetL], width - offsetL - offsetR,
                        window_weight(minI, maxI, -offsetL, offsetR), [&]( auto add ) {
                    for (int32_t i = minI; i <= maxI; i++) {
                        const E* src = input.data + input.offset + (y + i)*input.stride + offsetL;
                        const K* ker = kernel.data.data + (i + offsetL)*kw + offsetL;
                        for (int32_t j = -offsetL; j <= offsetR; j++)
                            add(src + j, ker[j]);
                    }
                });
            };
            for (int32_t y = (int32_t)y0; y < std::min((int32_t)y1, offsetL); y++)
                border_row(y, -y, offsetR);
            for (int32_t y = std::max((int32_t)y0, height - offsetR); y < (int32_t)y1; y++)
                border_row(y, -offsetL, height - y - 1);
        }

    private:
        /// Pixels summed at once by sum_row()
        static constexpr uint32_t CHUNK = 256;

        template<class E, class K>
        static E normalize( K total, K weight ) {
            if constexpr (std::is_integral<E>::value)
                return static_cast<E>((total + weight/2)/weight);
            else
                return static_cast<E>(total/weight);
        }

        /// Throws if an integer weight is zero
        template<class K>
        static void check_weight( K weight ) {
            if constexpr (std::is_integral<K>::value) {
                if (weight == 0)
                    throw std::domain_error("Kernel weights inside the image sum to zero");
            }
        }

        /// out[x] = (sum of src[x]*w over the terms)/weight for x < n. terms(add) calls add(src, w) once per term,
        /// in the order they're summed. Done CHUNK pixels at a time, as in ConvolveImage_Inner
        template<class E, class K, class Terms>
        static void sum_row( E* out, uint32_t n, K weight, Terms terms ) {
            for (uint32_t x0 = 0; x0 < n; x0 += CHUNK) {
                const uint32_t m = std::min(CHUNK, n - x0);
                K total[CHUNK];
                for (uint32_t x = 0; x < m; x++)
                    total[x] = 0;
                // offset the pointers here: GCC doesn't vectorize an index of x0 + x
                terms([&]( const E* src, K w ) {
                    src += x0;
                    for (uint32_t x = 0; x < m; x++)
                        total[x] += src[x]*w;
                });
                E* o = out + x0;
                for (uint32_t x = 0; x < m; x++)
                    o[x] = normalize<E>(total[x], weight);
            }
        }
    };


    /// Optimized convolution code which processes the inner portion of the image and doesn't handle the border. If
    /// the function has a divisor then the sum resulting from the convolution is divided by the divisor.
    ///
    /// Horizontal kernels whose width is only known at run time, vertical kernels and 2D kernels sum a block of
    /// pixels at once, so the compiler vectorizes across pixels:
    ///
    ///     for each output row:
    ///         for each block of 256 pixels:
    ///             totals = 0
    ///             for each kernel element k, in the order of one sum per pixel:  totals[x] += pixel(x, k)*weight[k]
    ///             out[x] = result(totals[x])
    ///
    /// Each pixel's sum adds the same terms in the same order as one sum per pixel, so the results are identical.
    class ConvolveImage_Inner {
    public:

        /// horizontal() with W set to the kernel's width for widths 3, 5, 7, 9, 11 and 13, so the loops over the kernel
        /// have a fixed length. Other widths use W = 0, the width at run time
        template<class E>
        static void horizontal_switch( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                       const Gray<E>& input, Gray<E>& output ,
                                       typename TypeInfo<E>::signed_type divisor)
        {
            switch( kernel.width ) {
                case 3:horizontal<3>(kernel,input,output,divisor);return;
                case 5:horizontal<5>(kernel,input,output,divisor);return;
                case 7:horizontal<7>(kernel,input,output,divisor);return;
                case 9:horizontal<9>(kernel,input,output,divisor);return;
                case 11:horizontal<11>(kernel,input,output,divisor);return;
                case 13:horizontal<13>(kernel,input,output,divisor);return;
            }
            horizontal(kernel,input,output,divisor);
        }

        /// Integer convolution where each sum is rounded and divided by 'divisor'. W is the kernel's width, or 0 if it's
        /// only known at run time
        template<int W = 0, class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            if( fits_mean_divisor<E>(kernel, divisor) ) {
                const MeanDivisor<E> mean((uint32_t)divisor);
                if( mean.uses_multiply32() )
                    horizontal_rows<W>(kernel, input, output, [mean]( signed_type total ) {
                        return mean.multiply32((sum_type)total); });
                else if( mean.uses_multiply() )
                    horizontal_rows<W>(kernel, input, output, [mean]( signed_type total ) {
                        return mean.multiply((sum_type)total); });
                else
                    horizontal_rows<W>(kernel, input, output, [mean]( signed_type total ) { return mean((sum_type)total); });
            } else {
                const signed_type halfDivisor = divisor/2;
                horizontal_rows<W>(kernel, input, output, [=]( signed_type total ) {
                    return static_cast<E>((total+halfDivisor)/divisor); });
            }
        }

        /// Convolution along each row, where 'result' turns each sum into the output pixel
        template<int W, class E, class R, class Result>
        static void horizontal_rows( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                     const Gray<E>& input, Gray<R>& output, Result result )
        {
            if constexpr (W == 0) {
                horizontal_rows_blocked(kernel, input, output, result);
                return;
            }
            typedef typename TypeInfo<E>::signed_type signed_type;
            const uint32_t width = (uint32_t)W;
            const signed_type* kernel_data = kernel.data.data;

            for( uint32_t i = 0; i < input.height; i++ ) {
                R* output_ptr = &output.data[output.offset + i*output.stride + kernel.offset];
                const E* input_row_ptr = &input.data[input.offset + i*input.stride];
                const uint32_t length = input.width-(width-1);

                for( uint32_t x = 0; x < length; x++ ) {
                    const E* input_ptr = &input_row_ptr[x];
                    signed_type total = 0;
                    for( uint32_t k = 0; k < width; k++ ) {
                        total += input_ptr[k] * kernel_data[k];
                    }
                    output_ptr[x] = result(total);
                }
            }
        }

        /// Kernels whose width is only known at run time, by blocks of pixels, see ConvolveImage_Inner.
        /// ConvolveNaive::horizontal is the simple equivalent
        template<class E, class R, class Result>
        static void horizontal_rows_blocked( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                             const Gray<E>& input, Gray<R>& output, Result result )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            constexpr uint32_t BLOCK = 256;
            const uint32_t length = input.width - (kernel.width - 1);
            signed_type totals[BLOCK];

            for( uint32_t i = 0; i < input.height; i++ ) {
                const E* input_row = &input.data[input.offset + i*input.stride];
                R* output_row = &output.data[output.offset + i*output.stride + kernel.offset];
                for( uint32_t x0 = 0; x0 < length; x0 += BLOCK ) {
                    const uint32_t n = std::min(BLOCK, length - x0);
                    std::fill_n(totals, n, signed_type(0));
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        const E* src = input_row + x0 + k;
                        const signed_type weight = kernel.data[k];
                        for( uint32_t x = 0; x < n; x++ )
                            totals[x] += src[x]*weight;
                    }
                    // offset the pointer here: GCC doesn't vectorize an index of x0 + x
                    R* out = output_row + x0;
                    for( uint32_t x = 0; x < n; x++ )
                        out[x] = result(totals[x]);
                }
            }
        }

        /// True if MeanDivisor gives the rounded division: the image is unsigned, the kernel has no negative weights,
        /// and the weights add up to at most 'divisor'. Then no sum is negative or larger than the largest pixel
        /// times 'divisor'
        template<class E>
        static bool fits_mean_divisor( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                       typename TypeInfo<E>::signed_type divisor ) {
            if( !std::is_unsigned<E>::value || divisor <= 0 )
                return false;
            int64_t sum = 0;
            for( uint32_t i = 0; i < kernel.width; i++ ) {
                if( kernel.data[i] < 0 )
                    return false;
                sum += kernel.data[i];
            }
            return sum <= divisor;
        }

        /// Floating point convolution where each sum is divided by 'divisor'. W is the kernel's width, or 0 if it's
        /// only known at run time
        template<int W = 0, class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            horizontal_rows<W>(kernel, input, output, [=]( signed_type total ) { return (E)(total/divisor); });
        }

        /// Same as horizontal_switch() above, for horizontal() without a divisor
        template<typename E, typename R>
        static void horizontal_switch( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                       const Gray<E>& input, Gray<R>& output )
        {
            switch( kernel.width ) {
                case 3:horizontal<3>(kernel,input,output);return;
                case 5:horizontal<5>(kernel,input,output);return;
                case 7:horizontal<7>(kernel,input,output);return;
                case 9:horizontal<9>(kernel,input,output);return;
                case 11:horizontal<11>(kernel,input,output);return;
                case 13:horizontal<13>(kernel,input,output);return;
            }
            horizontal(kernel,input,output);
        }

        /// Convolution without a divisor. W is the kernel's width, or 0 if it's only known at run time
        template<int W = 0, typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            horizontal_rows<W>(kernel, input, output, []( signed_type total ) { return static_cast<R>(total); });
        }

        /// vertical() with W set to the kernel's width for widths 3, 5, 7, 9, 11 and 13, so the loops over the kernel have
        /// a fixed length. Other widths use W = 0, the width at run time
        template<typename E>
        static void vertical_switch( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                     const Gray<E>& input, Gray<E>& output ,
                                     typename TypeInfo<E>::signed_type divisor )
        {
            switch( kernel.width ) {
                case 3:vertical<3>(kernel,input,output,divisor);return;
                case 5:vertical<5>(kernel,input,output,divisor);return;
                case 7:vertical<7>(kernel,input,output,divisor);return;
                case 9:vertical<9>(kernel,input,output,divisor);return;
                case 11:vertical<11>(kernel,input,output,divisor);return;
                case 13:vertical<13>(kernel,input,output,divisor);return;
            }
            vertical(kernel,input,output,divisor);
        }

        /// Integer convolution where each sum is rounded and divided by 'divisor'. W is the kernel's width, or 0 if it's
        /// only known at run time
        template<int W = 0, class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            if( fits_mean_divisor<E>(kernel, divisor) ) {
                const MeanDivisor<E> mean((uint32_t)divisor);
                if( mean.uses_multiply32() )
                    vertical_rows<W>(kernel, input, output, [mean]( signed_type total ) {
                        return mean.multiply32((sum_type)total); });
                else if( mean.uses_multiply() )
                    vertical_rows<W>(kernel, input, output, [mean]( signed_type total ) {
                        return mean.multiply((sum_type)total); });
                else
                    vertical_rows<W>(kernel, input, output, [mean]( signed_type total ) { return mean((sum_type)total); });
            } else {
                const signed_type halfDivisor = divisor/2;
                vertical_rows<W>(kernel, input, output, [=]( signed_type total ) {
                    return static_cast<E>((total+halfDivisor)/divisor); });
            }
        }

        /// Convolution along each column, where 'result' turns each sum into the output pixel. By blocks of pixels, see
        /// ConvolveImage_Inner
        template<int W, class E, class R, class Result>
        static void vertical_rows( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                   const Gray<E>& input, Gray<R>& output, Result result )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const uint32_t width = W > 0 ? (uint32_t)W : kernel.width;
            const signed_type* kernel_data = kernel.data.data;
            const uint32_t stride = input.stride;
            constexpr uint32_t BLOCK = 256;

            int32_t yEnd = input.height-(width-kernel.offset-1);

            for( int32_t y = kernel.offset; y < yEnd; y++ ) {
                R* output_ptr = &output.data[output.offset+y*output.stride];
                const E* input_row_ptr = &input.data[input.offset + (y-kernel.offset)*input.stride];

                for( uint32_t x0 = 0; x0 < input.width; x0 += BLOCK ) {
                    const uint32_t n = std::min(BLOCK, input.width - x0);
                    signed_type totals[BLOCK];
                    std::fill_n(totals, n, signed_type(0));
                    for( uint32_t k = 0; k < width; k++ ) {
                        const E* row = input_row_ptr + k*stride + x0;
                        const signed_type weight = kernel_data[k];
                        for( uint32_t x = 0; x < n; x++ )
                            totals[x] += row[x] * weight;
                    }
                    // offset the pointer here: GCC doesn't vectorize an index of x0 + x
                    R* out = output_ptr + x0;
                    for( uint32_t x = 0; x < n; x++ )
                        out[x] = result(totals[x]);
                }
            }
        }

        /// Same as vertical_rows, with each pixel's sum kept in a register. W must be known at compile time. Faster for
        /// floating point images with kernel widths 3 to 9; slower for wider kernels, where GCC no longer vectorizes
        /// it, and for integer images
        template<int W, class E, class R, class Result>
        static void vertical_rows_registers( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                             const Gray<E>& input, Gray<R>& output, Result result )
        {
            static_assert(W > 0, "kernel width must be known at compile time");
            typedef typename TypeInfo<E>::signed_type signed_type;
            signed_type weights[W];
            for( int k = 0; k < W; k++ )
                weights[k] = kernel.data[k];

            int32_t yEnd = input.height-(W-kernel.offset-1);

            for( int32_t y = kernel.offset; y < yEnd; y++ ) {
                R* out = &output.data[output.offset+y*output.stride];
                const E* rows[W];
                for( int k = 0; k < W; k++ )
                    rows[k] = &input.data[input.offset + (y-kernel.offset+k)*input.stride];
                for( uint32_t x = 0; x < input.width; x++ ) {
                    signed_type total = 0;
                    for( int k = 0; k < W; k++ )
                        total += rows[k][x] * weights[k];
                    out[x] = result(total);
                }
            }
        }

        /// Floating point convolution where each sum is divided by 'divisor'. W is the kernel's width, or 0 if it's
        /// only known at run time
        template<int W = 0, class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            auto result = [=]( signed_type total ) { return (E)(total/divisor); };
            if constexpr( W >= 3 && W <= 9 )
                vertical_rows_registers<W>(kernel, input, output, result);
            else
                vertical_rows<W>(kernel, input, output, result);
        }

        /// Same as vertical_switch() above, for vertical() without a divisor
        template<typename E, typename R>
        static void vertical_switch( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                     const Gray<E>& input, Gray<R>& output )
        {
            switch( kernel.width ) {
                case 3:vertical<3>(kernel,input,output);return;
                case 5:vertical<5>(kernel,input,output);return;
                case 7:vertical<7>(kernel,input,output);return;
                case 9:vertical<9>(kernel,input,output);return;
                case 11:vertical<11>(kernel,input,output);return;
                case 13:vertical<13>(kernel,input,output);return;
            }
            vertical(kernel,input,output);
        }

        /// Convolution without a divisor. W is the kernel's width, or 0 if it's only known at run time
        template<int W = 0, typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            auto result = []( signed_type total ) { return static_cast<R>(total); };
            if constexpr( std::is_floating_point<E>::value && W >= 3 && W <= 9 )
                vertical_rows_registers<W>(kernel, input, output, result);
            else
                vertical_rows<W>(kernel, input, output, result);
        }

        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const signed_type halfDivisor = divisor/2;
            convolve_blocked(kernel, input, output, [=]( signed_type total ) {
                return (E)((total+halfDivisor)/divisor); });
        }

        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            convolve_blocked(kernel, input, output, [=]( signed_type total ) { return (E)(total/divisor); });
        }

        /// convolve() without a divisor, with W set to the kernel's width for centered kernels 3, 5, 7, 9 and 11
        /// wide. Other kernels use W = 0
        template<typename E, typename R>
        static void convolve_switch( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                                     const Gray<E>& input, Gray<R>& output )
        {
            if( kernel.offset == kernel.width/2 ) {
                switch( kernel.width ) {
                    case 3: convolve<3>(kernel,input,output); return;
                    case 5: convolve<5>(kernel,input,output); return;
                    case 7: convolve<7>(kernel,input,output); return;
                    case 9: convolve<9>(kernel,input,output); return;
                    case 11: convolve<11>(kernel,input,output); return;
                }
            }
            convolve(kernel,input,output);
        }

        /// BoofCV's ConvolveImageUnrolled_SB_F32_F32.convolve{3,5,7,9,11}: each kernel row is summed separately,
        /// the first row is written to output and the following rows are added to it. W is the width of a centered
        /// kernel
        template<int W, typename E, typename R>
        BOOFCPP_NOINLINE static void convolve_rows( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                                                     const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const int32_t radius = W/2;
            const int32_t width = input.width;
            const int32_t height = input.height;

            for( int32_t y = radius; y < height-radius; y++ ) {
                for( int32_t i = 0; i < W; i++ ) {
                    const signed_type* k = &kernel.data.data[i*W];
                    const E* src = &input.data[input.offset + (y+i-radius)*input.stride];
                    R* dst = &output.data[output.offset + y*output.stride];

                    if( i == 0 ) {
                        for( int32_t x = radius; x < width-radius; x++ ) {
                            const E* s = &src[x-radius];
                            signed_type total = 0;
                            for( int32_t kj = 0; kj < W; kj++ )
                                total += s[kj]*k[kj];
                            dst[x] = (R)total;
                        }
                    } else {
                        for( int32_t x = radius; x < width-radius; x++ ) {
                            const E* s = &src[x-radius];
                            signed_type total = 0;
                            for( int32_t kj = 0; kj < W; kj++ )
                                total += s[kj]*k[kj];
                            dst[x] += (R)total;
                        }
                    }
                }
            }
        }

        /// Convolution without a divisor. W is the width of a centered kernel, or 0 if it's only known at run time. With
        /// a width it sums one kernel row at a time. Integer results don't depend on the order
        template<int W = 0, typename E, typename R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            if constexpr( W > 0 )
                convolve_rows<W>(kernel, input, output);
            else
                convolve_blocked(kernel, input, output, []( signed_type total ) { return (R)total; });
        }

        /// 2D convolution where 'result' turns each sum into the output pixel. By blocks of pixels, see
        /// ConvolveImage_Inner, with the kernel's rows, then columns. ConvolveNaive::convolve is the simple equivalent
        template<class E, class R, class Result>
        static void convolve_blocked( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                                      const Gray<E>& input, Gray<R>& output, Result result )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            constexpr uint32_t BLOCK = 256;
            const uint32_t width = kernel.width;
            const uint32_t offsetL = kernel.offset;
            const uint32_t offsetR = width - offsetL - 1;
            const uint32_t length = input.width - (width - 1);
            signed_type totals[BLOCK];

            for( uint32_t y = offsetL; y < input.height-offsetR; y++ ) {
                R* output_row = &output.data[output.offset + y*output.stride + offsetL];
                for( uint32_t x0 = 0; x0 < length; x0 += BLOCK ) {
                    const uint32_t n = std::min(BLOCK, length - x0);
                    std::fill_n(totals, n, signed_type(0));
                    for( uint32_t ki = 0; ki < width; ki++ ) {
                        const E* input_row = &input.data[input.offset + (y+ki-offsetL)*input.stride + x0];
                        for( uint32_t kj = 0; kj < width; kj++ ) {
                            const E* src = input_row + kj;
                            const signed_type weight = kernel.data[ki*width + kj];
                            for( uint32_t x = 0; x < n; x++ )
                                totals[x] += src[x]*weight;
                        }
                    }
                    // offset the pointer here: GCC doesn't vectorize an index of x0 + x
                    R* out = output_row + x0;
                    for( uint32_t x = 0; x < n; x++ )
                        out[x] = result(totals[x]);
                }
            }
        }
    };

    /// Convolution of the image border, where the kernel extends outside the image and pixels are read through
    /// ImageBorder. The functions without a range call the overload with a range, which computes a rectangle of output
    /// pixels, on each border strip. Concurrent code splits the strips
    class ConvolveImage_Border {
    public:
        /// Left and right borders of every row
        template<class E, class R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const ImageBorder<E>& input, Gray<R>& output )
        {
            const uint32_t borderRight = kernel.width-kernel.offset-1;
            horizontal(kernel, input, output, 0, kernel.offset, 0, output.height);
            horizontal(kernel, input, output, output.width-borderRight, output.width, 0, output.height);
        }

        /// Output pixels x0 <= x < x1 and y0 <= y < y1
        template<class E, class R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const ImageBorder<E>& input, Gray<R>& output,
                                uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const int32_t offset = kernel.offset;

            for( uint32_t y = y0; y < y1; y++ ) {
                R* output_row = &output.data[output.offset + y*output.stride];
                for( int32_t x = x0; x < (int32_t)x1; x++ ) {
                    signed_type total = 0;
                    for( int32_t k = 0; k < (int32_t)kernel.width; k++ )
                        total += input.get(x+k-offset,y) * kernel.data[k];
                    output_row[x] = static_cast<R>(total);
                }
            }
        }

        /// Top and bottom borders of every column
        template<class E, class R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            const uint32_t borderBottom = kernel.width-kernel.offset-1;
            vertical(kernel, input, output, 0, output.width, 0, kernel.offset);
            vertical(kernel, input, output, 0, output.width, output.height-borderBottom, output.height);
        }

        /// Output pixels x0 <= x < x1 and y0 <= y < y1
        template<class E, class R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output,
                              uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const int32_t offset = kernel.offset;

            for( int32_t y = y0; y < (int32_t)y1; y++ ) {
                R* output_row = &output.data[output.offset + y*output.stride];
                for( uint32_t x = x0; x < x1; x++ ) {
                    signed_type total = 0;
                    for( int32_t k = 0; k < (int32_t)kernel.width; k++ )
                        total += input.get(x,y+k-offset) * kernel.data[k];
                    output_row[x] = static_cast<R>(total);
                }
            }
        }

        /// Left and right borders of every row, then the top and bottom borders between them
        template<class E, class R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            const uint32_t offsetL = kernel.offset;
            const uint32_t offsetR = kernel.width-offsetL-1;
            const uint32_t width = output.width, height = output.height;
            convolve(kernel, input, output, 0, offsetL, 0, height);
            convolve(kernel, input, output, width-offsetR, width, 0, height);
            convolve(kernel, input, output, offsetL, width-offsetR, 0, offsetL);
            convolve(kernel, input, output, offsetL, width-offsetR, height-offsetR, height);
        }

        /// Output pixels x0 <= x < x1 and y0 <= y < y1
        template<class E, class R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output,
                              uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const int32_t offsetL = kernel.offset;
            const int32_t offsetR = kernel.width-offsetL-1;

            for( int32_t y = y0; y < (int32_t)y1; y++ ) {
                R* output_row = &output.data[output.offset + y*output.stride];
                for( int32_t x = x0; x < (int32_t)x1; x++ ) {
                    signed_type total = 0;
                    const signed_type* kernel_ptr = kernel.data.data;
                    for( int32_t i = -offsetL; i <= offsetR; i++ ) {
                        for( int32_t j = -offsetL; j <= offsetR; j++ )
                            total += input.get(x+j,y+i) * *kernel_ptr++;
                    }
                    output_row[x] = static_cast<R>(total);
                }
            }
        }
    };

    /**
     * Generalized functions for performing image convolution across an entire image.
     */
    class ConvolveImage {
    public:
        template<class E, class R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            if( kernel.width >= output.width ) {
                ConvolveNaive::horizontal(kernel, input, output);
            } else {
                ConvolveImage_Inner::horizontal_switch(kernel, *input.image, output);
                ConvolveImage_Border::horizontal(kernel, input, output);
            }
        }

        template<class E, class R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            if( kernel.width >= output.height ) {
                ConvolveNaive::vertical(kernel, input, output);
            } else {
                ConvolveImage_Inner::vertical_switch(kernel, *input.image, output);
                ConvolveImage_Border::vertical(kernel, input, output);
            }
        }

        template<class E, class R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            output.reshape(input.getWidth(),input.getHeight());

            if( kernel.width >= output.width || kernel.width >= output.height ) {
                ConvolveNaive::convolve(kernel, input, output);
            } else {
                ConvolveImage_Inner::convolve_switch(kernel, *input.image, output);
                ConvolveImage_Border::convolve(kernel, input, output);
            }
        };
    };

    /**
     * Optimized general normalized image convolution. Internally it invokes specialized code for handling image
     * border and the inner image.
     */
    class ConvolveNormalized {
    public:
        /**
         * BoofCV only rescales the kernel if its sum is not within 1e-4 of one (ConvolveImageNormalized).
         */
        template<class E>
        static bool needs_normalization( const Kernel1D<E>& kernel ) {
            E sum = 0;
            for( uint32_t i = 0; i < kernel.width; i++ ) sum += kernel.data[i];
            return std::abs(sum - (E)1) > (E)1e-4;
        }

        /**
         * BoofCV's KernelMath.normalizeSumToOne() applied to a copy of the kernel
         */
        template<class E>
        static void normalize_sum_to_one( const Kernel1D<E>& kernel, Kernel1D<E>& out ) {
            E sum = 0;
            for( uint32_t i = 0; i < kernel.width; i++ ) sum += kernel.data[i];
            for( uint32_t i = 0; i < kernel.width; i++ ) out.data[i] = kernel.data[i]/sum;
        }

        /// Integer kernels whose weights sum to zero, such as ones filled with zeros, throw. Floats aren't checked
        template<class K>
        static void check_sum( const KernelBase<K>& kernel ) {
            if constexpr (std::is_integral<K>::value) {
                if (kernel.sum() == 0)
                    throw std::domain_error("Kernel weights sum to zero");
            }
        }

        /// The kernel after check_sum(): for floats a copy in 'storage' that sums
        /// to one, unless the kernel's sum is within 1e-4 of one. Otherwise the kernel itself
        template<class K>
        static const Kernel1D<K>& normalized_kernel( const Kernel1D<K>& kernel, Kernel1D<K>& storage ) {
            check_sum(kernel);
            if constexpr (std::is_floating_point<K>::value) {
                if (needs_normalization(kernel)) {
                    storage = Kernel1D<K>(kernel.width, kernel.offset);
                    normalize_sum_to_one(kernel, storage);
                    return storage;
                }
            }
            return kernel;
        }

        /// normalized_kernel() for 2D kernels
        template<class K>
        static const Kernel2D<K>& normalized_kernel( const Kernel2D<K>& kernel, Kernel2D<K>& storage ) {
            check_sum(kernel);
            if constexpr (std::is_floating_point<K>::value) {
                K sum = 0;
                for (uint32_t i = 0; i < kernel.data.size; i++)
                    sum += kernel.data[i];
                if (std::abs(sum - (K)1) > (K)1e-4f) {
                    storage = Kernel2D<K>(kernel.width, kernel.offset);
                    for (uint32_t i = 0; i < kernel.data.size; i++)
                        storage.data[i] = kernel.data[i]/sum;
                    return storage;
                }
            }
            return kernel;
        }

        /// The inner image, with a kernel from normalized_kernel(). Integers divide by the kernel's sum
        template<class E>
        static void inner_horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input,
                                      Gray<E>& output ) {
            if constexpr (std::is_integral<E>::value)
                ConvolveImage_Inner::horizontal_switch(kernel, input, output, kernel.sum());
            else
                ConvolveImage_Inner::horizontal_switch(kernel, input, output);
        }

        /// See inner_horizontal()
        template<class E>
        static void inner_vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input,
                                    Gray<E>& output ) {
            if constexpr (std::is_integral<E>::value)
                ConvolveImage_Inner::vertical_switch(kernel, input, output, kernel.sum());
            else
                ConvolveImage_Inner::vertical_switch(kernel, input, output);
        }

        /// See inner_horizontal()
        template<class E>
        static void inner_convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input,
                                    Gray<E>& output ) {
            if constexpr (std::is_integral<E>::value)
                ConvolveImage_Inner::convolve(kernel, input, output, kernel.sum());
            else
                ConvolveImage_Inner::convolve_switch(kernel, input, output);
        }

        /// Normalized convolution along x. BoofCV's ConvolveImageNormalized.horizontal(). Kernels as wide as
        /// the image use the naive code, otherwise the inner image and the border are done separately
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input,
                                Gray<E>& output ) {
            typedef typename TypeInfo<E>::signed_type K;
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.width ) {
                ConvolveNormalizedNaive::horizontal(kernel, input, output);
            } else {
                Kernel1D<K> storage;
                const Kernel1D<K>& k = normalized_kernel(kernel, storage);
                inner_horizontal(k, input, output);
                ConvolveNormalized_JustBorder::horizontal(k, input, output);
            }
        }

        /// Normalized convolution along y. See horizontal()
        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input,
                              Gray<E>& output ) {
            typedef typename TypeInfo<E>::signed_type K;
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.height ) {
                ConvolveNormalizedNaive::vertical(kernel, input, output);
            } else {
                Kernel1D<K> storage;
                const Kernel1D<K>& k = normalized_kernel(kernel, storage);
                inner_vertical(k, input, output);
                ConvolveNormalized_JustBorder::vertical(k, input, output);
            }
        }

        /**
         * Applies a 1D kernel horizontally then vertically.
         * @tparam E Data type
         * @param kernel 1D kernel
         * @param input Input image that convolution is applied to
         * @param output Where the output is written to. resised
         * @param workspace Used to store intermediate results. Is resized
         */
        template<class E>
        static void convolve( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output, Gray<E>& workspace )
        {
            output.reshape(input.width,input.height);
            workspace.reshape(input.width,input.height);

            horizontal(kernel,input,workspace);
            vertical(kernel,workspace,output);
        }

        /// Normalized 2D convolution: each pixel is the kernel weighted sum of the pixels inside the image, divided by
        /// the sum of those weights. BoofCV's ConvolveImageNormalized.convolve(). Kernels as wide as the image
        /// use the naive code. Otherwise integers divide the inner image by the kernel's sum, and floats rescale the
        /// kernel to sum to one unless its sum is within 1e-4 of one
        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::signed_type signed_type;
            boofcv::checkSameShape(input, output);

            if (kernel.width >= input.width || kernel.width >= input.height) {
                ConvolveNormalizedNaive::convolve(kernel, input, output);
            } else {
                Kernel2D<signed_type> storage;
                const Kernel2D<signed_type>& k = normalized_kernel(kernel, storage);
                inner_convolve(k, input, output);
                ConvolveNormalized_JustBorder::convolve(k, input, output);
            }
        }
    };
}

#endif