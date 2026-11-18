#ifndef BOOFCPP_CONVOLVE_NAIVE_H
#define BOOFCPP_CONVOLVE_NAIVE_H

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include "boofcv/base_types.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/image_border.h"
#include "boofcv/image_types.h"

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

        /// Each pixel is the sum of the kernel weights inside the image times the pixels, divided by the sum of those
        /// weights. Integers round the division. Same as BoofCV's ConvolveNormalizedNaive_SB.convolve()
        template<class E>
        static void convolve( const Kernel2D<typename TypeInfo<E>::signed_type>& kernel,
                              const Gray<E>& input, Gray<E>& output ) {
            typedef typename TypeInfo<E>::signed_type signed_type;
            const int32_t offset = kernel.offset;
            const int32_t width = input.width;
            const int32_t height = input.height;

            for (int32_t y = 0; y < height; y++) {
                for (int32_t x = 0; x < width; x++) {
                    const int32_t startX = std::max(0, x - offset);
                    const int32_t endX = std::min(width, x - offset + (int32_t)kernel.width);
                    const int32_t startY = std::max(0, y - offset);
                    const int32_t endY = std::min(height, y - offset + (int32_t)kernel.width);

                    signed_type total = 0;
                    signed_type weight = 0;
                    for (int32_t i = startY; i < endY; i++) {
                        for (int32_t j = startX; j < endX; j++) {
                            signed_type v = kernel.at(j - x + offset, i - y + offset);
                            total += input.unsafe_at(j, i)*v;
                            weight += v;
                        }
                    }
                    if constexpr (std::is_integral<E>::value) {
                        // BoofCV's Java code throws an ArithmeticException
                        if (weight == 0)
                            throw std::domain_error("Kernel weights inside the image sum to zero");
                        output.unsafe_at(x, y) = static_cast<E>((total + weight/2)/weight);
                    }
                    else
                        output.unsafe_at(x, y) = static_cast<E>(total/weight);
                }
            }
        }
    };
}

#endif
