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

namespace boofcv {
    /// Speeds up integer division by changing it into a multiplication and a shift. Multiply can be vectorized
    /// while an integer division can’t. This approach should apply to any modern CPU architecture.
    ///
    /// Turns the sum of 'length' pixels into their mean, the same as BoofCV. Floats divide by the length and integers
    /// round to the nearest value, (total + length/2)/length.
    ///
    /// Dividing by a number that's only known at run time is slow. For unsigned integers the division is done with a
    /// multiplication and a shift that gives exactly the same result for every sum of 'length' pixels.
    template< class E>
    class MeanDivisor {
    public:
        typedef typename TypeInfo<E>::sum_type sum_type;

        explicit MeanDivisor( uint32_t length ) : divisor((sum_type)length) {
            if constexpr (std::is_unsigned<E>::value) {
                // Largest number that's divided. With n_max*length < 2^shift and multiplier = 2^shift/length + 1,
                // (n*multiplier) >> shift == n/length for every n <= n_max. n_max < 2^31 keeps n*multiplier < 2^64
                const uint64_t n_max = (uint64_t)std::numeric_limits<E>::max()*length + length/2;
                if (length == 0 || n_max >= (1ULL << 31))
                    return;
                const uint64_t limit = n_max*length;
                shift = 0;
                while (shift < 63 && (limit >> shift) != 0)
                    shift++;
                multiplier = ((uint64_t)1 << shift)/length + 1;
                use_multiply = true;
                // the product also fits in 32 bits, which can be computed for several pixels at once
                use_multiply32 = n_max*multiplier < (1ULL << 32) && shift < 32;
            }
        }

        E operator()( sum_type total ) const {
            if constexpr (std::is_floating_point<E>::value) {
                return static_cast<E>(total/divisor);
            } else {
                if constexpr (std::is_unsigned<E>::value) {
                    if (use_multiply)
                        return multiply(total);
                }
                return static_cast<E>((total + divisor/2)/divisor);
            }
        }

        /// True if operator() uses the multiplication instead of a division
        bool uses_multiply() const { return use_multiply; }

        /// Same as operator() when uses_multiply() is true
        E multiply( sum_type total ) const {
            return static_cast<E>(((uint64_t)(total + divisor/2)*multiplier) >> shift);
        }

        /// True if multiply32() can be used
        bool uses_multiply32() const { return use_multiply32; }

        /// Same as operator() when uses_multiply32() is true. The product is computed with 32 bits
        E multiply32( sum_type total ) const {
            return static_cast<E>(((uint32_t)(total + divisor/2)*(uint32_t)multiplier) >> shift);
        }

    private:
        sum_type divisor;
        bool use_multiply = false;
        bool use_multiply32 = false;
        uint64_t multiplier = 0;
        uint32_t shift = 0;
    };

    /**
     * Optimized normalized convlution code which just processes the image border.
     */
    class ConvolveNormalized_JustBorder {
    public:
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            uint32_t offsetL = kernel.offset;
            uint32_t offsetR = kernel.width-offsetL-1;

            for (uint32_t i = 0; i < input.height; i++) {
                uint32_t indexDest = output.offset + i * output.stride;
                uint32_t j = input.offset + i * input.stride;
                uint32_t jStart = j;
                uint32_t jEnd = j + offsetL;

                for (; j < jEnd; j++) {
                    signed_type total = 0;
                    signed_type weight = 0;
                    uint32_t indexSrc = jStart;
                    for (uint32_t k = kernel.width - (offsetR + 1 + j - jStart); k < kernel.width; k++) {
                        signed_type w = kernel.data[k];
                        weight += w;
                        total += input.data[indexSrc++] * w;
                    }
                    output.data[indexDest++] = (E)((total+weight/2)/weight);
                }

                j += input.width - (offsetL+offsetR);
                indexDest += input.width - (offsetL+offsetR);

                jEnd = jStart + input.width;
                for (; j < jEnd; j++) {
                    signed_type total = 0;
                    signed_type weight = 0;
                    uint32_t indexSrc = j - offsetL;
                    uint32_t kEnd = jEnd - indexSrc;

                    for (uint32_t k = 0; k < kEnd; k++) {
                        signed_type w = kernel.data[k];
                        weight += w;
                        total += input.data[indexSrc++] * w;
                    }
                    output.data[indexDest++] = (E)((total+weight/2)/weight);
                }
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
        {
            // Port of BoofCV ConvolveNormalized_JustBorder_SB.horizontal(Kernel1D_F32,...). The weight is updated
            // incrementally across the border, which changes floating point rounding vs. recomputing it each pixel.
            typedef typename TypeInfo<E>::signed_type signed_type;

            const signed_type* dataKer = kernel.data.data;
            const int32_t kernelWidth = kernel.width;
            const int32_t offsetL = kernel.offset;
            const int32_t offsetR = kernelWidth-offsetL-1;
            const int32_t width = input.width;

            for (uint32_t y = 0; y < input.height; y++) {
                int32_t indexDest = output.offset + y * output.stride;
                int32_t j = input.offset + y * input.stride;
                const int32_t jStart = j;
                int32_t jEnd = j + offsetL;

                signed_type weight = 0;
                for (int32_t k = kernelWidth - offsetR; k < kernelWidth; k++) {
                    weight += dataKer[k];
                }

                for (; j < jEnd; j++) {
                    signed_type total = 0;
                    int32_t indexSrc = jStart;
                    int32_t kernelIdx0 = kernelWidth - (offsetR + 1 + j - jStart);
                    weight += dataKer[kernelIdx0];
                    for (int32_t k = kernelIdx0; k < kernelWidth; k++) {
                        total += input.data[indexSrc++] * dataKer[k];
                    }
                    output.data[indexDest++] = (E)(total/weight);
                }

                j += width - (offsetL+offsetR);
                indexDest += width - (offsetL+offsetR);

                {
                    weight = 0;
                    int32_t kEnd = jStart + width - (j - offsetL) + 1;
                    for (int32_t k = 0; k < kEnd; k++) {
                        weight += dataKer[k];
                    }
                }

                jEnd = jStart + width;
                for (; j < jEnd; j++) {
                    signed_type total = 0;
                    int32_t indexSrc = j - offsetL;
                    const int32_t kEnd = jEnd - indexSrc;

                    for (int32_t k = 0; k < kEnd; k++) {
                        total += input.data[indexSrc++] * dataKer[k];
                    }
                    weight -= dataKer[kEnd];
                    output.data[indexDest++] = (E)(total/weight);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            uint32_t offsetL = kernel.offset;
            uint32_t offsetR = kernel.width-offsetL-1;

            uint32_t yEnd = input.height - offsetR;

            for (uint32_t y = 0; y < offsetL; y++) {
                uint32_t indexDst = output.offset + y * output.stride;
                uint32_t i = input.offset + y * input.stride;
                uint32_t iEnd = i + input.width;

                uint32_t kStart = offsetL - y;

                signed_type weight = 0;
                for (uint32_t k = kStart; k < kernel.width; k++) {
                    weight += kernel.data[k];
                }

                for ( ; i < iEnd; i++) {
                    signed_type total = 0;
                    uint32_t indexSrc = i - y * input.stride;
                    for (uint32_t k = kStart; k < kernel.width; k++, indexSrc += input.stride) {
                        total += input.data[indexSrc] * kernel.data[k];
                    }
                    output.data[indexDst++] = (E)((total+weight/2)/weight);
                }
            }

            for (uint32_t y = yEnd; y < input.height; y++) {
                uint32_t indexDst = output.offset + y * output.stride;
                uint32_t i = input.offset + y * input.stride;
                uint32_t iEnd = i + input.width;

                uint32_t kEnd = input.height - (y - offsetL);

                signed_type weight = 0;
                for (uint32_t k = 0; k < kEnd; k++) {
                    weight += kernel.data[k];
                }

                for ( ; i < iEnd; i++) {
                    signed_type total = 0;
                    uint32_t indexSrc = i - offsetL * input.stride;
                    for (uint32_t k = 0; k < kEnd; k++, indexSrc += input.stride) {
                        total += input.data[indexSrc] * kernel.data[k];
                    }
                    output.data[indexDst++] = (E)((total+weight/2)/weight);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            uint32_t offsetL = kernel.offset;
            uint32_t offsetR = kernel.width-offsetL-1;

            uint32_t yEnd = input.height - offsetR;

            for (uint32_t y = 0; y < offsetL; y++) {
                uint32_t indexDst = output.offset + y * output.stride;
                uint32_t i = input.offset + y * input.stride;
                uint32_t iEnd = i + input.width;

                uint32_t kStart = offsetL - y;

                signed_type weight = 0;
                for (uint32_t k = kStart; k < kernel.width; k++) {
                    weight += kernel.data[k];
                }

                for ( ; i < iEnd; i++) {
                    signed_type total = 0;
                    uint32_t indexSrc = i - y * input.stride;
                    for (uint32_t k = kStart; k < kernel.width; k++, indexSrc += input.stride) {
                        total += input.data[indexSrc] * kernel.data[k];
                    }
                    output.data[indexDst++] = (E)(total/weight);
                }
            }

            for (uint32_t y = yEnd; y < input.height; y++) {
                uint32_t indexDst = output.offset + y * output.stride;
                uint32_t i = input.offset + y * input.stride;
                uint32_t iEnd = i + input.width;

                uint32_t kEnd = input.height - (y - offsetL);

                signed_type weight = 0;
                for (uint32_t k = 0; k < kEnd; k++) {
                    weight += kernel.data[k];
                }

                for ( ; i < iEnd; i++) {
                    signed_type total = 0;
                    uint32_t indexSrc = i - offsetL * input.stride;
                    for (uint32_t k = 0; k < kEnd; k++, indexSrc += input.stride) {
                        total += input.data[indexSrc] * kernel.data[k];
                    }
                    output.data[indexDst++] = (E)(total/weight);
                }
            }
        }
    };


    /**
     * Optimized convolution code which processes the inner portion of the image
     * and doesn't handle the border. if the function has a divisor then the sum
     * resulting from the convolution is divided by the divisor
     */
    class ConvolveImage_Inner {
    public:

        template<class E>
        static void horizontal_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
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
                    horizontal_rows<W>(kernel, input, output, [&]( signed_type total ) {
                        return mean.multiply32((sum_type)total); });
                else if( mean.uses_multiply() )
                    horizontal_rows<W>(kernel, input, output, [&]( signed_type total ) {
                        return mean.multiply((sum_type)total); });
                else
                    horizontal_rows<W>(kernel, input, output, [&]( signed_type total ) { return mean((sum_type)total); });
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
            typedef typename TypeInfo<E>::signed_type signed_type;
            const uint32_t width = W > 0 ? (uint32_t)W : kernel.width;
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

        template<typename E, typename R>
        static void horizontal_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
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

        template<typename E>
        static void vertical_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
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
                    vertical_rows<W>(kernel, input, output, [&]( signed_type total ) {
                        return mean.multiply32((sum_type)total); });
                else if( mean.uses_multiply() )
                    vertical_rows<W>(kernel, input, output, [&]( signed_type total ) {
                        return mean.multiply((sum_type)total); });
                else
                    vertical_rows<W>(kernel, input, output, [&]( signed_type total ) { return mean((sum_type)total); });
            } else {
                const signed_type halfDivisor = divisor/2;
                vertical_rows<W>(kernel, input, output, [=]( signed_type total ) {
                    return static_cast<E>((total+halfDivisor)/divisor); });
            }
        }

        /// Convolution along each column, where 'result' turns each sum into the output pixel
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

                // Columns are processed in blocks. Each column's sum adds the rows in the same order as one sum
                // per pixel would, while each row is read for many columns at once
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
                    for( uint32_t x = 0; x < n; x++ )
                        output_ptr[x0 + x] = result(totals[x]);
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
            vertical_rows<W>(kernel, input, output, [=]( signed_type total ) { return (E)(total/divisor); });
        }

        template<typename E, typename R>
        static void vertical_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
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
            vertical_rows<W>(kernel, input, output, []( signed_type total ) { return static_cast<R>(total); });
        }

        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            signed_type halfDivisor = divisor/2;

            int32_t offsetL = kernel.offset;
            int32_t offsetR = kernel.width-kernel.offset-1;

            for( uint32_t y = offsetL; y < input.height-offsetR; y++ ) {
                uint32_t indexDst = output.offset + y*output.stride+offsetL;
                for( uint32_t x = offsetL; x < input.width-offsetR; x++ ) {
                    signed_type total = 0;
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t ki = 0; ki < kernel.width; ki++ ) {
                        E* input_ptr = &input.data[input.offset + (y+ki-offsetL)*input.stride + x-offsetL];
                        for( uint32_t kj = 0; kj <  kernel.width; kj++ ) {
                            total += *input_ptr++ * *kernel_ptr++;
                        }
                    }
                    output.data[indexDst++] = (E)((total+halfDivisor)/divisor);
                }
            }
        }

        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            int32_t offsetL = kernel.offset;
            int32_t offsetR = kernel.width-kernel.offset-1;

            for( uint32_t y = offsetL; y < input.height-offsetR; y++ ) {
                uint32_t indexDst = output.offset + y*output.stride+offsetL;
                for( uint32_t x = offsetL; x < input.width-offsetR; x++ ) {
                    signed_type total = 0;
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t ki = 0; ki < kernel.width; ki++ ) {
                        E* input_ptr = &input.data[input.offset + (y+ki-offsetL)*input.stride + x-offsetL];
                        for( uint32_t kj = 0; kj <  kernel.width; kj++ ) {
                            total += *input_ptr++ * *kernel_ptr++;
                        }
                    }
                    output.data[indexDst++] = (E)(total/divisor);
                }
            }
        }

        /**
         * Mirrors BoofCV ConvolveImageUnrolled_SB_F32_F32.convolve{3,5,7,9,11}: each kernel row is summed separately,
         * the first row is written to output and the following rows are added to it.
         * @return true if the kernel width is supported and the image was processed
         */
        template<typename E, typename R>
        static bool convolve_rows( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                                   const Gray<E>& input, Gray<R>& output )
        {
            if( kernel.offset != kernel.width/2 )
                return false;
            switch( kernel.width ) {
                case 3: convolve_rows_w<3>(kernel,input,output); return true;
                case 5: convolve_rows_w<5>(kernel,input,output); return true;
                case 7: convolve_rows_w<7>(kernel,input,output); return true;
                case 9: convolve_rows_w<9>(kernel,input,output); return true;
                case 11: convolve_rows_w<11>(kernel,input,output); return true;
                default: return false;
            }
        }

        template<int W, typename E, typename R>
        BOOFCPP_NOINLINE static void convolve_rows_w( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
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

        template<typename E, typename R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            // BoofCV uses unrolled code for these kernel widths, which sums one kernel row at a time. Same order is used
            // here so floating point results are bit identical. Integer results don't depend on the order
            if( convolve_rows(kernel,input,output) )
                return;

            uint32_t offsetL = kernel.offset;
            uint32_t offsetR = kernel.width-kernel.offset-1;

            for( uint32_t y = offsetL; y < input.height-offsetR; y++ ) {
                uint32_t indexDst = output.offset + y*output.stride+offsetL;
                for( uint32_t x = offsetL; x < input.width-offsetR; x++ ) {
                    signed_type total = 0;
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t ki = 0; ki < kernel.width; ki++ ) {
                        E* input_ptr = &input.data[input.offset + (y+ki-offsetL)*input.stride + x-offsetL];
                        for( uint32_t kj = 0; kj <  kernel.width; kj++ ) {
                            total += (*input_ptr++) * (*kernel_ptr++);
                        }
                    }
                    output.data[indexDst++] = (R)total;
                }
            }
        }
    };

    /**
     * Convolution code which can handle the image border. This code is difficult to optimize
     * because of the special case which need to be handled. Which is why it's handled
     * seperately from the inner image which can be highly optimized.
     */
    class ConvolveImage_Border {
    public:
        template<class E, class R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const ImageBorder<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            int32_t offset = kernel.offset;
            int32_t borderRight = kernel.width-offset-1;

            for (uint32_t y = 0; y < output.height; y++)
            {
                R* output_ptr = &output.data[output.offset + y * output.stride];
                for ( int32_t x = 0; x < offset; x++ ) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for (int32_t k = 0; k < kernel.width; k++) {
                        total += input.get(x+k-offset,y) * *kernel_ptr++;
                    }
                    *output_ptr++ = static_cast<R>(total);
                }

                output_ptr = &output.data[output.offset + y * output.stride + output.width-borderRight];
                for ( int32_t x = output.width-borderRight; x < output.width; x++ ) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    int32_t xx = x-offset; // No noticeable change. Keeping it here to avoid doing it again.
                    int32_t xx_end = xx + kernel.width;
                    while( xx != xx_end ) {
                        total += input.get(xx++,y) * *kernel_ptr++;
                    }
                    *output_ptr++ = static_cast<R>(total);
                }
            }
        }

        template<class E, class R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            int32_t borderBottom = kernel.width-kernel.offset-1;

            for ( uint32_t x = 0; x < output.width; x++ ) {
                uint32_t indexDest = output.offset + x;

                for (uint32_t y = 0; y < kernel.offset; y++, indexDest += output.stride) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        total += input.get(x,y+k-kernel.offset) * *kernel_ptr++;
                    }
                    output.data[indexDest] = static_cast<R>(total);
                }

                indexDest = output.offset + (output.height-borderBottom) * output.stride + x;
                for (int32_t y = output.height-borderBottom; y < output.height; y++, indexDest += output.stride) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for (uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.get(x,y+k-kernel.offset) * *kernel_ptr++;
                    }
                    output.data[indexDest] = static_cast<R>(total);
                }
            }
        }

        template<class E, class R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            // signed so that logic of inner loops isn't messed up
            int32_t offsetL = kernel.offset;
            int32_t offsetR = kernel.width-offsetL-1;

            // convolve along the left and right borders
            for (uint32_t y = 0; y < output.height; y++) {
                uint32_t indexDest = output.offset + y * output.stride;

                for ( int32_t x = 0; x < offsetL; x++ ) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for( int32_t i = -offsetL; i <= offsetR; i++ ) {
                        for (int32_t j = -offsetL; j <= offsetR; j++) {
                            total += input.get(x+j,y+i) * *kernel_ptr++;
                        }
                    }
                    output.data[indexDest++] = static_cast<R>(total);
                }

                indexDest = output.offset + y * output.stride + output.width-offsetR;
                for ( int32_t x = output.width-offsetR; x < output.width; x++ ) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for( int32_t i = -offsetL; i <= offsetR; i++ ) {
                        for (int32_t j = -offsetL; j <= offsetR; j++) {
                            total += input.get(x+j,y+i) * *kernel_ptr++;
                        }
                    }
                    output.data[indexDest++] = static_cast<R>(total);
                }
            }

            // convolve along the top and bottom borders
            for ( uint32_t x = offsetL; x < output.width-offsetR; x++ ) {
                uint32_t indexDest = output.offset + x;

                for (int32_t y = 0; y < offsetL; y++, indexDest += output.stride) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for( int32_t i = -offsetL; i <= offsetR; i++ ) {
                        for (int32_t j = -offsetL; j <= offsetR; j++) {
                            total += input.get(x+j,y+i) * *kernel_ptr++;
                        }
                    }
                    output.data[indexDest] = static_cast<R>(total);
                }

                indexDest = output.offset + (output.height-offsetR) * output.stride + x;
                for (int32_t y = output.height-offsetR; y < output.height; y++, indexDest += output.stride) {
                    signed_type total = 0;
                    signed_type *kernel_ptr = kernel.data.data;
                    for( int32_t i = -offsetL; i <= offsetR; i++ ) {
                        for (int32_t j = -offsetL; j <= offsetR; j++) {
                            total += input.get(x+j,y+i) * *kernel_ptr++;
                        }
                    }
                    output.data[indexDest] = static_cast<R>(total);
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
                ConvolveImage_Inner::horizontal_unrolled(kernel, *input.image, output);
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
                ConvolveImage_Inner::vertical_unrolled(kernel, *input.image, output);
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
                ConvolveImage_Inner::convolve(kernel, *input.image, output);
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
         * Same as BoofCV KernelMath.normalizeSumToOne() applied to a copy of the kernel
         */
        template<class E>
        static void normalize_sum_to_one( const Kernel1D<E>& kernel, Kernel1D<E>& out ) {
            E sum = 0;
            for( uint32_t i = 0; i < kernel.width; i++ ) sum += kernel.data[i];
            for( uint32_t i = 0; i < kernel.width; i++ ) out.data[i] = kernel.data[i]/sum;
        }
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.width ) {
                ConvolveNormalizedNaive::horizontal(kernel, input, output);
            } else {
                ConvolveImage_Inner::horizontal_unrolled(kernel, input, output, kernel.sum());
                ConvolveNormalized_JustBorder::horizontal(kernel, input, output);
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.width ) {
                ConvolveNormalizedNaive::horizontal(kernel, input, output);
            } else {
                // Same as BoofCV ConvolveImageNormalized: kernel is only rescaled when its sum is not within 1e-4
                // of one, then a regular convolution is applied to the inner image
                if( needs_normalization(kernel) ) {
                    Kernel1D<signed_type> k(kernel.width, kernel.offset);
                    normalize_sum_to_one(kernel, k);
                    ConvolveImage_Inner::horizontal_unrolled(k, input, output);
                    ConvolveNormalized_JustBorder::horizontal(k, input, output);
                } else {
                    ConvolveImage_Inner::horizontal_unrolled(kernel, input, output);
                    ConvolveNormalized_JustBorder::horizontal(kernel, input, output);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.height ) {
                ConvolveNormalizedNaive::vertical(kernel, input, output);
            } else {
                ConvolveImage_Inner::vertical_unrolled(kernel, input, output, kernel.sum());
                ConvolveNormalized_JustBorder::vertical(kernel, input, output);
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.height ) {
                ConvolveNormalizedNaive::vertical(kernel, input, output);
            } else {
                if( needs_normalization(kernel) ) {
                    Kernel1D<signed_type> k(kernel.width, kernel.offset);
                    normalize_sum_to_one(kernel, k);
                    ConvolveImage_Inner::vertical_unrolled(k, input, output);
                    ConvolveNormalized_JustBorder::vertical(k, input, output);
                } else {
                    ConvolveImage_Inner::vertical_unrolled(kernel, input, output);
                    ConvolveNormalized_JustBorder::vertical(kernel, input, output);
                }
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
    };
}

#endif