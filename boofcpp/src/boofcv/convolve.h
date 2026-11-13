#ifndef BOOFCPP_CONVOLVE_H
#define BOOFCPP_CONVOLVE_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <initializer_list>
#include <random>

#include "base_types.h"
#include "image_types.h"
#include "sanity_checks.h"
#include "image_border.h"
#include "convolve_kernels.h"

namespace boofcv {
    /**
     * Unoptimized image convolution algorithms. They are intended to be easy to understand and can
     * be verified through manual inspection. They can also handle all edge cases, e.g. kernel larger than
     * the input image.
     */
    class ConvolveNaive {
    public:
        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const ImageBorder<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t xx = x - kernel.offset + k;
                        sum += kernel.at(k) * input.get(xx, y);
                    }
                    output.at(x, y) = (R)sum;
                }
            }
        }

        template<typename E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<E>& output , typename TypeInfo<E>::signed_type divisor,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t xx = x - kernel.offset + k;
                        sum += kernel.at(k) * input.get(xx, y);
                    }
                    output.at(x, y) = (E)((sum + divisor/2)/divisor);
                }
            }
        }

        template<typename E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<E>& output , typename TypeInfo<E>::signed_type divisor,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t xx = x - kernel.offset + k;
                        sum += kernel.at(k) * input.get(xx, y);
                    }
                    output.at(x, y) = (E)(sum/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const ImageBorder<E>& input, Gray<R>& output ) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        sum += kernel.at(k) * input.get(x, yy);
                    }
                    output.at(x, y) = (R)sum;
                }
            }
        }

        template<typename E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<E>& output , typename TypeInfo<E>::signed_type divisor,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        sum += kernel.at(k) * input.get(x, yy);
                    }
                    output.at(x, y) = (E)((sum + divisor/2)/divisor);
                }
            }
        }

        template<typename E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<E>& output , typename TypeInfo<E>::signed_type divisor,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        sum += kernel.at(k) * input.get(x, yy);
                    }
                    output.at(x, y) = (E)(sum/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const ImageBorder<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        for (uint32_t l = 0; l < kernel.width; l++) {
                            int32_t xx = x - kernel.offset + l;
                            sum += kernel.at(l,k) * input.get(xx, yy);
                        }
                    }
                    output.at(x, y) = (R)sum;
                }
            }
        }

        template<typename E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const ImageBorder<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        for (uint32_t l = 0; l < kernel.width; l++) {
                            int32_t xx = x - kernel.offset + l;
                            sum += kernel.at(l,k) * input.get(xx, yy);
                        }
                    }
                    output.at(x, y) = (E)((sum + divisor/2)/divisor);
                }
            }
        }

        template<typename E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const ImageBorder<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.image->height; y++) {
                for (uint32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (uint32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        for (uint32_t l = 0; l < kernel.width; l++) {
                            int32_t xx = x - kernel.offset + l;
                            sum += kernel.at(l,k) * input.get(xx, yy);
                        }
                    }
                    output.at(x, y) = (E)(sum/divisor);
                }
            }
        }

    };

    /**
     * Unoptimized normalizing convolution algorithms. They are intended to be easy to understand and work no matter
     * what you throw at it, e.g. kernel larger than the image.
     */
    class ConvolveNormalizedNaive {
    public:
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    int32_t startX,endX;
                    if( x < kernel.offset ) {
                        startX = 0;
                        endX = kernel.width-(kernel.offset-x);
                    } else {
                        startX = x - kernel.offset;
                        endX = startX + kernel.width;
                    }

                    if( endX > input.width ) endX = input.width;

                    signed_type total = 0;
                    signed_type weight = 0;

                    for( int32_t j = startX; j < endX; j++ ) {
                        signed_type v = kernel[j-x+kernel.offset];
                        total += input.unsafe_at(j,y)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = static_cast<E>((total+weight/2)/weight);
                }
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                                const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    int32_t startX,endX;
                    if( x < kernel.offset ) {
                        startX = 0;
                        endX = kernel.width-(kernel.offset-x);
                    } else {
                        startX = x - kernel.offset;
                        endX = startX + kernel.width;
                    }

                    if( endX > input.width ) endX = input.width;

                    signed_type total = 0;
                    signed_type weight = 0;

                    for( int32_t j = startX; j < endX; j++ ) {
                        signed_type v = kernel[j-x+kernel.offset];
                        total += input.unsafe_at(j,y)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = static_cast<E>(total/weight);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    int32_t startY,endY;
                    if( y < kernel.offset ) {
                        startY = 0;
                        endY = kernel.width-(kernel.offset-y);
                    } else {
                        startY = y - kernel.offset;
                        endY = startY + kernel.width;
                    }

                    if( endY > input.height ) endY = input.height;

                    signed_type total = 0;
                    signed_type weight = 0;

                    for( int32_t i = startY; i < endY; i++ ) {
                        E v = kernel.at(i-y+kernel.offset);
                        total += input.unsafe_at(x,i)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = static_cast<E>((total+weight/2)/weight);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    int32_t startY,endY;
                    if( y < kernel.offset ) {
                        startY = 0;
                        endY = kernel.width-(kernel.offset-y);
                    } else {
                        startY = y - kernel.offset;
                        endY = startY + kernel.width;
                    }

                    if( endY > input.height ) endY = input.height;

                    signed_type total = 0;
                    signed_type weight = 0;

                    for( int32_t i = startY; i < endY; i++ ) {
                        E v = kernel.at(i-y+kernel.offset);
                        total += input.unsafe_at(x,i)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = static_cast<E>(total/weight);
                }
            }
        }

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
                case 3:horizontal(kernel,input,output,divisor);return;
                case 5:horizontal(kernel,input,output,divisor);return;
                case 7:horizontal(kernel,input,output,divisor);return;
                case 9:horizontal(kernel,input,output,divisor);return;
                case 11:horizontal(kernel,input,output,divisor);return;
                case 13:horizontal(kernel,input,output,divisor);return;
            }
            horizontal(kernel,input,output,divisor);
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            signed_type halfDivisor = divisor/2;

            for( uint32_t i = 0; i < input.height; i++ ) {
                E* output_ptr = &output.data[output.offset + i*output.stride + kernel.offset];
                E* input_row_ptr = &input.data[input.offset + i*input.stride];
                E* input_end_ptr = &input_row_ptr[input.width-(kernel.width-1)];

                while( input_row_ptr != input_end_ptr) {
                    E* input_ptr = input_row_ptr++;
                    signed_type* kernel_ptr = kernel.data.data;
                    signed_type* kernel_end = &kernel_ptr[kernel.width];

                    signed_type total = 0;
                    while( kernel_ptr != kernel_end ) {
                        total += (*input_ptr++) * (*kernel_ptr++);
                    }
                    *output_ptr++ = static_cast<E>((total+halfDivisor)/divisor);
                }
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            for( uint32_t i = 0; i < input.height; i++ ) {
                uint32_t indexDst = output.offset + i*output.stride + kernel.offset;
                int32_t j = input.offset + i*input.stride;
                int32_t jEnd = j+input.width-(kernel.width-1);

                for( ; j < jEnd; j++ ) {
                    signed_type total = 0;
                    E* input_ptr = &input.data[j];
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t k = kernel.width; k; k-- ) {
                        total += (*input_ptr++) * (*kernel_ptr++);
                    }
                    output.data[indexDst++] = (E)(total/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void horizontal_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                         const Gray<E>& input, Gray<R>& output )
        {
            switch( kernel.width ) {
                case 3:horizontal(kernel,input,output);return;
                case 5:horizontal(kernel,input,output);return;
                case 7:horizontal(kernel,input,output);return;
                case 9:horizontal(kernel,input,output);return;
                case 11:horizontal(kernel,input,output);return;
                case 13:horizontal(kernel,input,output);return;
            }
            horizontal(kernel,input,output);
        }

        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            for( uint32_t i = 0; i < input.height; i++ ) {
                R* output_ptr = &output.data[output.offset + i*output.stride + kernel.offset];
                E* input_row_ptr = &input.data[input.offset + i*input.stride];
                E* input_end_ptr = &input_row_ptr[input.width-(kernel.width-1)];

                while( input_row_ptr != input_end_ptr) {
                    E* input_ptr = input_row_ptr++;
                    signed_type* kernel_ptr = kernel.data.data;
                    signed_type* kernel_end = &kernel_ptr[kernel.width];

                    signed_type total = 0;
                    while( kernel_ptr != kernel_end ) {
                        total += (*input_ptr++) * (*kernel_ptr++);
                    }
                    *output_ptr++ = static_cast<R>(total);
                }
            }
        }

        template<typename E>
        static void vertical_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                       const Gray<E>& input, Gray<E>& output ,
                                       typename TypeInfo<E>::signed_type divisor )
        {
            switch( kernel.width ) {
                case 3:vertical(kernel,input,output,divisor);return;
                case 5:vertical(kernel,input,output,divisor);return;
                case 7:vertical(kernel,input,output,divisor);return;
                case 9:vertical(kernel,input,output,divisor);return;
                case 11:vertical(kernel,input,output,divisor);return;
                case 13:vertical(kernel,input,output,divisor);return;
            }
            vertical(kernel,input,output,divisor);
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            signed_type halfDivisor = divisor/2;

            int32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( int32_t y = kernel.offset; y < yEnd; y++ ) {
                E* output_ptr = &output.data[output.offset+y*output.stride];

                E* input_col_ptr = &input.data[input.offset + (y-kernel.offset)*input.stride];
                E* input_end_ptr = &input_col_ptr[input.width];

                while( input_col_ptr != input_end_ptr ){
                    E* input_ptr = input_col_ptr++;
                    signed_type* kernel_ptr = kernel.data.data;
                    signed_type* kernel_end = &kernel_ptr[kernel.width];

                    signed_type total = 0;
                    while( kernel_ptr != kernel_end ) {
                        total += (*input_ptr) * (*kernel_ptr++);
                        input_ptr += input.stride;
                    }
                    *output_ptr++ = static_cast<E>((total+halfDivisor)/divisor);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            uint32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( uint32_t y = kernel.offset; y < yEnd; y++ ) {
                uint32_t indexDst = output.offset+y*output.stride;
                int32_t i = input.offset + (y-kernel.offset)*input.stride;
                int32_t iEnd = i+input.width;

                for( ; i < iEnd; i++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = i;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc]* kernel.data[k];
                        indexSrc += input.stride;
                    }
                    output.data[indexDst++] = (E)(total/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void vertical_unrolled( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                       const Gray<E>& input, Gray<R>& output )
        {
            switch( kernel.width ) {
                case 3:vertical(kernel,input,output);return;
                case 5:vertical(kernel,input,output);return;
                case 7:vertical(kernel,input,output);return;
                case 9:vertical(kernel,input,output);return;
                case 11:vertical(kernel,input,output);return;
                case 13:vertical(kernel,input,output);return;
            }
            vertical(kernel,input,output);
        }

        template<typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<R>& output )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            int32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( int32_t y = kernel.offset; y < yEnd; y++ ) {
                R* output_ptr = &output.data[output.offset+y*output.stride];

                E* input_col_ptr = &input.data[input.offset + (y-kernel.offset)*input.stride];
                E* input_end_ptr = &input_col_ptr[input.width];

                while( input_col_ptr != input_end_ptr ){
                    E* input_ptr = input_col_ptr++;
                    signed_type* kernel_ptr = kernel.data.data;
                    signed_type* kernel_end = &kernel_ptr[kernel.width];

                    signed_type total = 0;
                    while( kernel_ptr != kernel_end ) {
                        total += (*input_ptr) * (*kernel_ptr++);
                        input_ptr += input.stride;
                    }
                    *output_ptr++ = static_cast<R>(total);
                }
            }
        }

        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, Gray<E>& output ,
                              typename TypeInfo<E>::signed_type divisor ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
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
            typedef typename TypeInfo<E>::sum_type sum_type;

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
            typedef typename TypeInfo<E>::sum_type sum_type;

            // Floating point: BoofCV uses unrolled code for these kernel widths, which sums one kernel row at a time.
            // Same order is used here so results are bit identical. Integer results don't depend on the order.
            if( std::is_floating_point<E>::value && convolve_rows(kernel,input,output) )
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
            typedef typename TypeInfo<E>::sum_type sum_type;

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
            typedef typename TypeInfo<E>::sum_type sum_type;

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
            typedef typename TypeInfo<E>::sum_type sum_type;

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