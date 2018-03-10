#ifndef BOOFCPP_CONVOLVE_KERNELS_H
#define BOOFCPP_CONVOLVE_KERNELS_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <initializer_list>
#include <random>

#include "base_types.h"
#include "image_types.h"

namespace boofcv {
    /**
     * Base type for all convolutional kernels
     * @tparam E
     */
    template< class E>
    class KernelBase {
    public:
        typedef typename TypeInfo<E>::sum_type sum_type;

        /* number of elements in the kernel */
        uint32_t width;

        /** which index is the kernel's origin.  For symmetric kernels with an odd width it is width/2 */
        uint32_t offset;

        /** Storage for kernel data */
        GrowArray<E> data;

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

        virtual void reshape( uint32_t width , uint32_t offset ) = 0;

        uint32_t radius() const {
            return width/2;
        }

        virtual uint32_t dimension() const = 0;

        sum_type sum() const {
            sum_type total = 0;
            for( uint32_t i = 0; i < data.size; i++ ) {
                total += data[i];
            }
            return total;
        }

        E element_max() const {
            E max = this->data.at(0);
            for( uint32_t i = 1; i < this->data.size; i++ ) {
                const E&a = this->data[i];
                if( a > max ) {
                    max = a;
                }
            }
            return max;
        }

        E element_min() const {
            E min = this->data.at(0);
            for( uint32_t i = 1; i < this->data.size; i++ ) {
                const E&a = this->data[i];
                if( a < min ) {
                    min = a;
                }
            }
            return min;
        }

        E element_abs_max() const {
            E max = std::abs(this->data.at(0));
            for( uint32_t i = 1; i < this->data.size; i++ ) {
                const E&a = std::abs(this->data[i]);
                if( a > max ) {
                    max = a;
                }
            }
            return max;
        }

        E element_abs_min() const {
            E min = std::abs(this->data.at(0));
            for( uint32_t i = 1; i < this->data.size; i++ ) {
                const E&a = std::abs(this->data[i]);
                if( a < min ) {
                    min = a;
                }
            }
            return min;
        }
    };

    /**
     * 1D Kernel. Used to convolve in the horizontal and vertical directions.
     */
    template< class E>
    class Kernel1D : public KernelBase<E> {
    public:

        Kernel1D( uint32_t width , uint32_t offset) : KernelBase<E>(width,offset) {
            this->data.resize(width);
        }

        Kernel1D( uint32_t width ) : Kernel1D(width,width/2) {
        }

        Kernel1D( uint32_t offset, std::initializer_list<E> l ) : KernelBase<E>((uint32_t)l.size(),offset) {
            this->data.setTo(l);
        }

        Kernel1D() {
        }

        void reshape( uint32_t width , uint32_t offset ) override {
            this->width = width;
            this->offset = offset;
            this->data.resize(width);
        }

        E& at( uint32_t index ) const {
            return this->data.at(index);
        }

        E& operator[] (uint32_t index) const {
            return this->data[index];
        }

        uint32_t dimension() const override  {
            return 1;
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

    /**
     * 2D Kernel. Convolves a N by N region.
     */
    template< class E>
    class Kernel2D : public KernelBase<E> {
    public:
        Kernel2D( uint32_t width , uint32_t offset) : KernelBase<E>(width,offset) {
            this->data.resize(width*width);
        }

        Kernel2D( uint32_t width ) : Kernel2D(width,width/2) {
        }

        Kernel2D(uint32_t offset, std::initializer_list<std::initializer_list<E>> l ) {
            this->width = static_cast<uint32_t >(l.size());
            this->offset = offset;

            this->data.resize(this->width*this->width);

            E* ptr = this->data.data;
            for (auto& x : l ) {
                if( x.size() != this->width ) {
                    throw invalid_argument("missmatched row width");
                }
                for(auto& y : x ) {
                    *ptr++ = y;
                }
            }
        }

        Kernel2D() {
        }

        void reshape( uint32_t width , uint32_t offset ) override {
            this->width = width;
            this->offset = offset;
            this->data.resize(width*width);
        }

        uint32_t dimension() const override  {
            return 2;
        }

        E& at( uint32_t x , uint32_t y ) const {
            if(x >= this->width || y >= this->width )
                throw invalid_argument("Out of bounds error");
            return this->data.at(y*this->width+x);
        }

        Kernel2D<E>& operator=(const Kernel2D<E>& other) // copy assignment
        {
            if (this != &other) { // self-assignment check expected
                this->width = other.width;
                this->offset = other.offset;
                this->data = other.data;
            }
            return *this;
        }
    };

    /**
     * Used to create commonly used convolution kernels.
     */
    class FactoryKernel {
    public:
        static constexpr double PI = 3.14159265358979;
        static constexpr double SQRT_2_PI = 2.5066282746310;//std::sqrt(2*PI);

        /**
         * Creates a 1D kernel where all values are 1
         * @param width How wide the kernel is
         */
        template<class E>
        static Kernel1D<E> table1D( uint32_t width ) {
            Kernel1D<E> k(width);
            for( uint32_t i = 0; i < width; i++ ) {
                k.data[i] = 1;
            }
            return k;
        }

        /**
         * A kernel where each element has the same value and the sum of the elements
         * adds up to one.
         * @param width How wide the kernel is

         */
        template<class E>
        static Kernel1D<E> mean1D( uint32_t width ) {
            Kernel1D<E> mean = table1D<E>(width);
            normalize(mean);
            return mean;
        }

        template<class E>
        static void normalize( KernelBase<E>& kernel , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {
            // do nothing. Work around for template
        }

        /**
         * Scales the kernel so that it's sum adds up to one.
         */
        template<class E>
        static void normalize( KernelBase<E>& kernel , typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 ) {

            typename Kernel1D<E>::sum_type sum = kernel.sum();

            for( uint32_t i = 0; i < kernel.width; i++ ) {
                kernel.data[i] /= sum;
            }
        }


        /**
         * <p>
         * Given the the radius of a Gaussian distribution and the order of its derivative, choose an appropriate sigma.
         * </p>
         * @param radius Kernel's radius
         * @param order Order of the derivative.  0 original distribution
         * @return Default sigma
         */
        static double sigmaForRadius(double radius , int32_t order ) {
            if( radius <= 0 )
                throw invalid_argument("Radius must be > 0");

            return (radius*2.0 + 1.0) / (5.0+0.8*order);
        }

        /**
         * <p>
         * Given the the sigma of a Gaussian distribution and the order of its derivative, choose an appropriate radius.
         * </p>
         *
         * @param sigma Distribution's sigma
         * @param order Order of the derivative.  0 original distribution
         * @return Default sigma
         */
        static int32_t radiusForSigma(double sigma, int32_t order ) {
            if( sigma <= 0 )
                throw invalid_argument("Sigma must be > 0");

            return (int32_t)ceil((((5+0.8*order)*sigma)-1)/2);
        }

        /**
         * <p>
         * Creates a floating point Gaussian kernel with the sigma and radius.
         * If normalized is set to true then the elements in the kernel will sum up to one.
         * </p>
         * @param sigma     Distributions standard deviation.
         * @param width    Kernel's width.
         * @param odd Does the kernel have an even or add width
         * @param normalize If the kernel should be normalized to one or not.
         */
        template< class E >
        static Kernel1D<E> gaussian1D(double sigma, int32_t width,
                                      typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 )
        {
            if( width <= 0 )
                width = radiusForSigma(sigma,0)*2+1;
            else if( sigma <= 0 )
                sigma = sigmaForRadius(width/2,0);

            Kernel1D<E> ret((uint32_t)width);
            int radius = width/2;

            double adjust = width%2==1?0:0.5;
            for (int i = 0; i < width; i++) {
                ret.data[i] = static_cast<E>(computePDF(0, sigma, i-radius+adjust));
            }

            normalize(ret);

            return ret;
        }

        template< class E >
        static Kernel2D<E> gaussian2D(double sigma, int32_t width,
                                      typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 )
        {
            Kernel1D<E> seed = gaussian1D<E>(sigma,width);
            Kernel2D<E> output(width,width/2);

            for( int i = 0; i < width; i++ ) {
                for( int j = 0; j < width; j++ ) {
                    output.at(i,j) = seed.at(i)*seed.at(j);
                }
            }

            normalize(output);

            return output;
        }

        /**
         * <p>
         * Creates a floating point Gaussian kernel with the sigma and radius.
         * If normalized is set to true then the elements in the kernel will sum up to one.
         * </p>
         * @param sigma Distributions standard deviation.
         * @param width Kernel's width.
         * @param odd Does the kernel have an even or add width
         * @param normalize If the kernel should be normalized to one or not.
         */
        template< class E>
        static Kernel1D<E> gaussian1D(double sigma, int32_t width,
                                      typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {
            Kernel1D<F64> kernel_float = gaussian1D<F64>(sigma, width);

            return float_to_int<E>(kernel_float,1.0/100.0);
        }

        template< class E>
        static Kernel2D<E> gaussian2D(double sigma, int32_t width,
                                      typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {
            Kernel2D<F64> kernel_float = gaussian2D<F64>(sigma, width);

            return float_to_int<E>(kernel_float,1.0/100.0);
        }


        static double computePDF( double mean , double sigma , double sample ) {
            double delta = sample-mean;

            return std::exp( - delta*delta/(2.0*sigma*sigma))/(sigma*SQRT_2_PI);
        }

        /**
         * Converts a float kernel into an interger kernel
         * @tparam E
         * @param original
         * @param minFrac
         * @return
         */
        template< class E>
        static Kernel1D<E> float_to_int( const Kernel1D<F64>& original , F64 minFrac,
                                         typename std::enable_if<std::is_integral<E>::value>::type* = 0)
        {

            Kernel1D<E> output(original.width, original.offset);

            F64 max = original.element_abs_max();
            F64 min = std::max(original.element_abs_min(),max*minFrac);

            for( int i = 0; i < original.width; i++ ) {
                output[i] = static_cast<E>(original[i]/min);
            }

            return output;
        }

        template< class E>
        static Kernel2D<E> float_to_int( const Kernel2D<F64>& original , F64 minFrac,
                                         typename std::enable_if<std::is_integral<E>::value>::type* = 0)
        {

            Kernel2D<E> output(original.width, original.offset);

            F64 max = original.element_abs_max();
            F64 min = std::max(original.element_abs_min(),max*minFrac);

            for( uint32_t i = 0; i < original.width; i++ ) {
                for( uint32_t j = 0; j < original.width; j++ ) {
                    output.at(j,i) = static_cast<E>(original.at(j,i) / min);
                }
            }

            return output;
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

        template<class E, class RNG>
        static void fill_uniform( Kernel2D<E>& kernel , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            std::uniform_int_distribution<E> dis(min_value, max_value-1);

            uint32_t N = kernel.width*kernel.width;
            for( uint32_t i = 0; i < N; i++ ) {
                kernel.data[i] = dis(rng);
            }
        }

        template<class E, class RNG>
        static void fill_uniform( Kernel2D<E>& kernel , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {

            std::uniform_real_distribution<E> dis(min_value, max_value);

            uint32_t N = kernel.width*kernel.width;
            for( uint32_t i = 0; i < N; i++ ) {
                kernel.data[i] = dis(rng);
            }
        }
    };

}

#endif