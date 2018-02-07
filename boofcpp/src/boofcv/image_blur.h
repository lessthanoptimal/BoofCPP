#ifndef BOOFCPP_IMAGE_BLUR_H
#define BOOFCPP_IMAGE_BLUR_H

#include "convolve.h"

namespace boofcv {
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

                sum_type total = 0;

                uint32_t indexEnd = indexIn + kernelWidth;

                for( ; indexIn < indexEnd; indexIn++ ) {
                    total += input.data[indexIn];
                }
                output.data[indexOut++] = static_cast<E>((total+halfDivisor)/divisor);

                indexEnd = indexIn + input.width - kernelWidth;
                for( ; indexIn < indexEnd; indexIn++ ) {
                    total -= input.data[ indexIn - kernelWidth ];
                    total += input.data[ indexIn ];

                    output.data[indexOut++] = static_cast<E>((total+halfDivisor)/divisor);
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

                sum_type total = 0;

                uint32_t indexEnd = indexIn + kernelWidth;

                for( ; indexIn < indexEnd; indexIn++ ) {
                    total += input.data[indexIn];
                }
                output.data[indexOut++] = static_cast<E>(total/divisor);

                indexEnd = indexIn + input.width - kernelWidth;
                for( ; indexIn < indexEnd; indexIn++ ) {
                    total -= input.data[ indexIn - kernelWidth ];
                    total += input.data[ indexIn ];

                    output.data[indexOut++] = static_cast<E>(total/divisor);
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

                for( uint32_t x = 0; x < input.width; x++ ,indexIn++,indexOut++) {
                    sum_type total = totals[ x ]  - input.data[ indexIn - backStep ];
                    totals[ x ] = total += input.data[ indexIn ];

                    output.data[indexOut] = static_cast<E>((total+halfDivisor)/divisor);
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

                for( uint32_t x = 0; x < input.width; x++ ,indexIn++,indexOut++) {
                    sum_type total = totals[ x ]  - input.data[ indexIn - backStep ];
                    totals[ x ] = total += input.data[ indexIn ];

                    output.data[indexOut] = static_cast<E>(total/divisor);
                }
            }
        }
    };
}

#endif