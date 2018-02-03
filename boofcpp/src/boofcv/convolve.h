#ifndef BOOFCPP_CONVOLVE_H
#define BOOFCPP_CONVOLVE_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <initializer_list>

#include "base_types.h"
#include "image_types.h"

namespace boofcv {

    class KernelBase {
    public:
        /* number of elements in the kernel */
        uint32_t width;

        /** which index is the kernel's origin.  For symmetric kernels with an odd width it is width/2 */
        uint32_t offset;

        KernelBase() {
            width = 0;
            offset = 0;
        }

        KernelBase( uint32_t width , uint32_t offset) {
            if( offset >= width )
                throw std::invalid_argument("The offset must be inside the kernel's bounds");
            this->width = width;
            this->offset = offset;
        }

        uint32_t radius() const {
            return width/2;
        }

        virtual uint32_t dimension() const = 0;
    };

    template< class E>
    class Kernel1D : public KernelBase {
    public:
        GrowArray<E> data;

        Kernel1D( uint32_t width , uint32_t offset) : KernelBase(width,offset) {
            data.resize(width);
        }

        Kernel1D( uint32_t width ) : Kernel1D(width,width/2) {
        }

        Kernel1D( uint32_t offset, std::initializer_list<E> l ) : KernelBase((uint32_t)l.size(),offset) {
            data.setTo(l);
        }

        E& at( uint32_t index ) const {
            return data.at(index);
        }

        E& operator[] (uint32_t index) const {
            return data[index];
        }

        uint32_t dimension() const override  {
            return 1;
        }
    };

    template< class E>
    class Kernel2D : public KernelBase {
    public:
        GrowArray<E> data;

        Kernel2D( uint32_t width , uint32_t offset) : KernelBase(width,offset) {
            data.resize(width*width);
        }

        Kernel2D( uint32_t width ) : Kernel2D(width,width/2) {
        }

        uint32_t dimension() const override  {
            return 2;
        }
    };


    class ConvolveNormalizedNaive {
    public:
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    uint32_t startX,endX;
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

                    for( uint32_t j = startX; j < endX; j++ ) {
                        signed_type v = kernel[j-x+kernel.offset];
                        total += input.unsafe_at(j,y)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = (total+weight/2)/weight;
                }
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    uint32_t startX,endX;
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

                    for( uint32_t j = startX; j < endX; j++ ) {
                        signed_type v = kernel[j-x+kernel.offset];
                        total += input.unsafe_at(j,y)*v;
                        weight += v;
                    }
                    output.unsafe_at(x,y) = total/weight;
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    uint32_t startY,endY;
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
                    output.unsafe_at(x,y) = (total+weight/2)/weight;
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (uint32_t y = 0; y < input.height; y++) {
                for( uint32_t x = 0; x < input.width; x++ ) {
                    uint32_t startY,endY;
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
                    output.unsafe_at(x,y) = total/weight;
                }
            }
        }

    };

    class ConvolveNormalized {
    public:

    };

    class ConvolveImageMean {
    public:
        template< class E>
        static void horizontal(const Gray<E>& input, Gray<E>& output, uint32_t radius) {

        }

        template< class E>
        static void vertical(const Gray<E>& input, Gray<E>& output, uint32_t radius) {

        }
    };
}

#endif