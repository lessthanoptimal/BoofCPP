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
        typedef typename TypeInfo<E>::sum_type sum_type;
        GrowArray<E> data;

        Kernel1D( uint32_t width , uint32_t offset) : KernelBase(width,offset) {
            data.resize(width);
        }

        Kernel1D( uint32_t width ) : Kernel1D(width,width/2) {
        }

        Kernel1D( uint32_t offset, std::initializer_list<E> l ) : KernelBase((uint32_t)l.size(),offset) {
            data.setTo(l);
        }

        Kernel1D() {
        }

        void reshape( uint32_t width , uint32_t offset ) {
            this->width = width;
            this->offset = offset;
            this->data.resize(width);
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

        sum_type sum() const {
            sum_type total = 0;
            for( uint32_t i = 0; i < width; i++ ) {
                total += data[i];
            }
            return total;
        }

        Kernel1D<E>& operator=(const Kernel1D<E>& other) // copy assignment
        {
            if (this != &other) { // self-assignment check expected
                this->width = other.width;
                this->offset = other.offset;
                this->data = other.data;
            }
            return *this;
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

    class FactoryKernel {
    public:
        template<class E>
        static Kernel1D<E> table1D( uint32_t width ) {
            Kernel1D<E> k(width);
            for( uint32_t i = 0; i < width; i++ ) {
                k.data[i] = 1;
            }
            return k;
        }

        template<class E>
        static Kernel1D<E> mean1D( uint32_t width ) {
            Kernel1D<E> mean = table1D<E>(width);
            normalize(mean);
            return mean;
        }

        template<class E>
        static void normalize( Kernel1D<E>& kernel , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {
            // do nothing. Work around for template
        }

        template<class E>
        static void normalize( Kernel1D<E>& kernel , typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 ) {

            typename Kernel1D<E>::sum_type sum = kernel.sum();


            for( uint32_t i = 0; i < kernel.width; i++ ) {
                kernel.data[i] /= sum;
            }
        }
    };

    template< class E>
    class ImageBorder {
    public:
        const Gray<E>* image;

        void setImage( const Gray<E>& image ) {
            this->image = &image;
        }

        E get( int32_t x , int32_t y) const {
            if( image->isInBounds(x,y)) {
                return image->at(x,y);
            }
            return outside_get(x,y);
        }

        virtual E outside_get( int32_t x , int32_t y ) const = 0;
    };

    template< class E>
    class ImageBorderValue : public ImageBorder<E> {
    public:
        E value;

        ImageBorderValue( E value ) {
            this->value = value;
        }

        E outside_get( int32_t x , int32_t y ) const override {
            return this->value;
        }
    };

    class KernelOps
    {
    public:
        /**
         * Fills an integer gray image using a uniform distribution.
         *
         * @param image Image that's to be filled
         * @param min_value minimum value (inclusive)
         * @param max_value  maximum value (exclusive)
         * @param rng The random number generator, e.g. std::mt19937
         */
        template<class E, class RNG>
        static void fill_uniform( Kernel1D<E>& kernel , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            std::uniform_int_distribution<E> dis(min_value, max_value-1);

            for( uint32_t i = 0; i < kernel.width; i++ ) {
                kernel.data[i] = dis(rng);
            }
        }

        template<class E, class RNG>
        static void fill_uniform( Kernel1D<E>& kernel , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            std::uniform_real_distribution<E> dis(min_value, max_value);

            for( uint32_t i = 0; i < kernel.width; i++ ) {
                kernel.data[i] = dis(rng);
            }
        }
    };

    class ConvolveNaive {
    public:
        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<R>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
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

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
                        int32_t xx = x - kernel.offset + k;
                        sum += kernel.at(k) * input.get(xx, y);
                    }
                    output.at(x, y) = (E)((sum + divisor/2)/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<R>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
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
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
                        int32_t xx = x - kernel.offset + k;
                        sum += kernel.at(k) * input.get(xx, y);
                    }
                    output.at(x, y) = (E)(sum/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<R>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
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

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        sum += kernel.at(k) * input.get(x, yy);
                    }
                    output.at(x, y) = (E)((sum + divisor/2)/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const ImageBorder<E>& input,
                                Gray<R>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
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
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            typedef typename TypeInfo<E>::signed_type signed_type;

            for (int32_t y = 0; y < input.image->height; y++) {
                for (int32_t x = 0; x < input.image->width; x++) {

                    signed_type sum = 0;
                    for (int32_t k = 0; k < kernel.width; k++) {
                        int32_t yy = y - kernel.offset + k;
                        sum += kernel.at(k) * input.get(x, yy);
                    }
                    output.at(x, y) = (E)(sum/divisor);
                }
            }
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
                    output.unsafe_at(x,y) = static_cast<E>((total+weight/2)/weight);
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
                    output.unsafe_at(x,y) = static_cast<E>(total/weight);
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
                    output.unsafe_at(x,y) = static_cast<E>((total+weight/2)/weight);
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
                    output.unsafe_at(x,y) = static_cast<E>(total/weight);
                }
            }
        }

    };

    class ConvolveNormalized_JustBorder {
    public:
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
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
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0)
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
                    output.data[indexDest++] = (E)(total/weight);
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
                    output.data[indexDest++] = (E)(total/weight);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
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
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
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


    class ConvolveImage_Inner {
    public:

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            signed_type halfDivisor = divisor/2;

            for( uint32_t i = 0; i < input.height; i++ ) {
                uint32_t indexDst = output.offset + i*output.stride + kernel.offset;
                uint32_t j = input.offset + i*input.stride;
                uint32_t jEnd = j+input.width-(kernel.width-1);

                for( ; j < jEnd; j++ ) {
                    signed_type total = 0;
                    E* input_ptr = &input.data[j];
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t k = kernel.width; k > 0; k-- ) {
                        total += (*input_ptr++) * (*kernel_ptr++);
                    }
                    output.data[indexDst++] = (E)((total+halfDivisor)/divisor);
                }
            }
        }

        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            for( uint32_t i = 0; i < input.height; i++ ) {
                uint32_t indexDst = output.offset + i*output.stride + kernel.offset;
                uint32_t j = input.offset + i*input.stride;
                uint32_t jEnd = j+input.width-(kernel.width-1);

                for( ; j < jEnd; j++ ) {
                    signed_type total = 0;
                    E* input_ptr = &input.data[j];
                    signed_type* kernel_ptr = kernel.data.data;
                    for( uint32_t k = kernel.width-1; k > 0; k-- ) {
                        total += (*input_ptr++) * (*kernel_ptr++);
                    }
                    output.data[indexDst++] = (E)(total/divisor);
                }
            }
        }

        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<R>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            for( uint32_t i = 0; i < input.height; i++ ) {
                uint32_t indexDst = output.offset + i*output.stride + kernel.offset;
                uint32_t j = input.offset + i*input.stride;
                uint32_t jEnd = j+input.width-(kernel.width-1);

                for( ; j < jEnd; j++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = j;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc++] * kernel.data[k];
                    }
                    output.data[indexDst++] = (R)total;
                }
            }
        }

        template<typename E, typename R>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<R>& output ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            for( uint32_t i = 0; i < input.height; i++ ) {
                uint32_t indexDst = output.offset + i*output.stride + kernel.offset;
                uint32_t j = input.offset + i*input.stride;
                uint32_t jEnd = j+input.width-(kernel.width-1);

                for( ; j < jEnd; j++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = j;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc++] * kernel.data[k];
                    }
                    output.data[indexDst++] = (R)total;
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;
            signed_type halfDivisor = divisor/2;

            uint32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( uint32_t y = kernel.offset; y < yEnd; y++ ) {
                uint32_t indexDst = output.offset+y*output.stride;
                uint32_t i = input.offset + (y-kernel.offset)*input.stride;
                uint32_t iEnd = i+input.width;

                for( ; i < iEnd; i++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = i;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc]* kernel.data[k];
                        indexSrc += input.stride;
                    }
                    output.data[indexDst++] = (E)((total+halfDivisor)/divisor);
                }
            }
        }

        template<class E>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                                const Gray<E>& input, const Gray<E>& output ,
                                typename TypeInfo<E>::signed_type divisor ,
                                typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            uint32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( uint32_t y = kernel.offset; y < yEnd; y++ ) {
                uint32_t indexDst = output.offset+y*output.stride;
                uint32_t i = input.offset + (y-kernel.offset)*input.stride;
                uint32_t iEnd = i+input.width;

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
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, const Gray<R>& output ,
                              typename std::enable_if<std::is_integral<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            uint32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( uint32_t y = kernel.offset; y < yEnd; y++ ) {
                uint32_t indexDst = output.offset+y*output.stride;
                uint32_t i = input.offset + (y-kernel.offset)*input.stride;
                uint32_t iEnd = i+input.width;

                for( ; i < iEnd; i++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = i;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc]* kernel.data[k];
                        indexSrc += input.stride;
                    }
                    output.data[indexDst++] = (R)total;
                }
            }
        }

        template<typename E, typename R>
        static void vertical( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel ,
                              const Gray<E>& input, const Gray<R>& output ,
                              typename std::enable_if<std::is_floating_point<E>::value >::type* = 0 )
        {
            typedef typename TypeInfo<E>::signed_type signed_type;
            typedef typename TypeInfo<E>::sum_type sum_type;

            uint32_t yEnd = input.height-(kernel.width-kernel.offset-1);

            for( uint32_t y = kernel.offset; y < yEnd; y++ ) {
                uint32_t indexDst = output.offset+y*output.stride;
                uint32_t i = input.offset + (y-kernel.offset)*input.stride;
                uint32_t iEnd = i+input.width;

                for( ; i < iEnd; i++ ) {
                    signed_type total = 0;
                    uint32_t indexSrc = i;
                    for( uint32_t k = 0; k < kernel.width; k++ ) {
                        total += input.data[indexSrc]* kernel.data[k];
                        indexSrc += input.stride;
                    }
                    output.data[indexDst++] = (R)total;
                }
            }
        }
    };

    class ConvolveNormalized {
    public:
        template<class E>
        static void horizontal( const Kernel1D<typename TypeInfo<E>::signed_type>& kernel, const Gray<E>& input, Gray<E>& output ,
                                typename std::enable_if<std::is_integral<E>::value >::type* = 0)
        {
            boofcv::checkSameShape(input, output);

            if( kernel.width >= input.width ) {
                ConvolveNormalizedNaive::horizontal(kernel, input, output);
            } else {
                ConvolveImage_Inner::horizontal(kernel, input, output, kernel.sum());
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
                signed_type sum = kernel.sum();
                if( sum == (signed_type)1) {
                    ConvolveImage_Inner::horizontal(kernel, input, output);
                } else {
                    ConvolveImage_Inner::horizontal(kernel, input, output, sum);
                }
                ConvolveNormalized_JustBorder::horizontal(kernel, input, output);
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
                ConvolveImage_Inner::vertical(kernel, input, output, kernel.sum());
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
                signed_type sum = kernel.sum();
                if( sum == (signed_type)1) {
                    ConvolveImage_Inner::vertical(kernel, input, output);
                } else {
                    ConvolveImage_Inner::vertical(kernel, input, output, sum);
                }
                ConvolveNormalized_JustBorder::vertical(kernel, input, output);
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