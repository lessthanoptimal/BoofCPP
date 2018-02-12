#ifndef BOOFCPP_IMAGE_BLUR_H
#define BOOFCPP_IMAGE_BLUR_H

#include "convolve.h"
#include "sanity_checks.h"

namespace boofcv {
    /**
     * Functions for applying a mean convolution kernel across the image. Speeds up the operation by performing
     * a horizontal and vertical 1D convolution.
     */
    class ConvolveImageMean {
    public:
        template< class E>
        static void inner_horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius ,
                                     typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::sum_type sum_type;
            uint32_t kernelWidth = radius*2 + 1;

            sum_type divisor = kernelWidth;
            sum_type halfDivisor = divisor/2;

            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t indexIn = input.offset + input.stride*y;
                uint32_t indexOut = output.offset + output.stride*y + radius;
                E* ptr_out = &output.data[indexOut];

                sum_type total = 0;

                uint32_t indexEnd = indexIn + kernelWidth;

                for( ; indexIn < indexEnd; indexIn++ ) {
                    total += input.data[indexIn];
                }
                *ptr_out++ = static_cast<E>((total+halfDivisor)/divisor);

                E* ptr_front = &input.data[ indexIn - kernelWidth ];
                E* ptr_back = &input.data[ indexIn ];
                uint32_t N = input.width - kernelWidth;

                for( uint32_t i = N; i > 0; i-- ) {
                    total -= *ptr_front++;
                    total += *ptr_back++;

                    *ptr_out++ = static_cast<E>((total+halfDivisor)/divisor);
                }
            }
        }

        template< class E>
        static void inner_horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius ,
                                     typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::sum_type sum_type;
            uint32_t kernelWidth = radius*2 + 1;

            sum_type divisor = kernelWidth;

            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t indexIn = input.offset + input.stride*y;
                uint32_t indexOut = output.offset + output.stride*y + radius;
                E* ptr_out = &output.data[indexOut];

                sum_type total = 0;

                uint32_t indexEnd = indexIn + kernelWidth;

                for( ; indexIn < indexEnd; indexIn++ ) {
                    total += input.data[indexIn];
                }
                *ptr_out++ = static_cast<E>(total/divisor);

                E* ptr_front = &input.data[ indexIn - kernelWidth ];
                E* ptr_back = &input.data[ indexIn ];
                uint32_t N = input.width - kernelWidth;

                for( uint32_t i = N; i > 0; i-- ) {
                    total -= *ptr_front++;
                    total += *ptr_back++;

                    *ptr_out++ = static_cast<E>(total/divisor);
                }
            }
        }

        template< class E>
        static void inner_vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius,
                                   typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::sum_type sum_type;
            sum_type kernelWidth = radius*2 + 1;

            uint32_t backStep = kernelWidth*input.stride;

            sum_type divisor = kernelWidth;
            sum_type halfDivisor = divisor/2;
            sum_type totals[ input.width ];

            for( uint32_t x = 0; x < input.width; x++ ) {
                uint32_t indexIn = input.offset + x;
                uint32_t indexOut = output.offset + output.stride*radius + x;

                sum_type total = 0;
                uint32_t indexEnd = indexIn + input.stride*kernelWidth;
                for( ; indexIn < indexEnd; indexIn += input.stride) {
                    total += input.data[indexIn];
                }
                totals[x] = total;
                output.data[indexOut] = static_cast<E>((total+halfDivisor)/divisor);
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

                    *ptr_out++ = static_cast<E>((total+halfDivisor)/divisor);
                }
            }
        }

        template< class E>
        static void inner_vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius,
                                   typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
        {
            typedef typename TypeInfo<E>::sum_type sum_type;
            sum_type kernelWidth = radius*2 + 1;

            uint32_t backStep = kernelWidth*input.stride;

            sum_type divisor = kernelWidth;
            sum_type totals[ input.width ];

            for( uint32_t x = 0; x < input.width; x++ ) {
                uint32_t indexIn = input.offset + x;
                uint32_t indexOut = output.offset + output.stride*radius + x;

                sum_type total = 0;
                uint32_t indexEnd = indexIn + input.stride*kernelWidth;
                for( ; indexIn < indexEnd; indexIn += input.stride) {
                    total += input.data[indexIn];
                }
                totals[x] = total;
                output.data[indexOut] = static_cast<E>(total/divisor);
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

                    *ptr_out++ = static_cast<E>(total/divisor);
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
            if( kernel.width > input.width ) {
                ConvolveNormalized::vertical(kernel,input,output);
            } else {
                ConvolveNormalized_JustBorder::vertical(kernel, input ,output );
                inner_vertical(input, output, radius);
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

            if (radius <= 0)
                throw invalid_argument("Radius must be > 0");

            output.reshape(input.width, input.height);
            storage.reshape(input.width, input.height);

            ConvolveImageMean::horizontal(input, storage, radius);
            ConvolveImageMean::vertical(storage, output, radius);
        }
    };
}

#endif