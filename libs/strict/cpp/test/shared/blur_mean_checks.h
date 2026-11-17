#ifndef BOOFCPP_BLUR_MEAN_CHECKS_H
#define BOOFCPP_BLUR_MEAN_CHECKS_H

/// Checks of tested::BlurImageOps::mean, shared by every variant. Include a variant header, e.g. variant_strict.h,
/// first. Each variant's tests call check_mean_all() for the image types it supports. Results must be within
/// floating point tolerance of strict's naive convolution, and identical for integers

#include "boofcv/convolve.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <random>

namespace blur_mean_checks {
    using namespace boofcv;

    /// Mean along x and then y, from naive convolution with mean kernels
    template<class E>
    void naive_mean( const Gray<E>& input, uint32_t radius_x, uint32_t radius_y, Gray<E>& output ) {
        typedef typename TypeInfo<E>::signed_type signed_type;
        Kernel1D<signed_type> kernel_x = FactoryKernel::mean1D<signed_type>(radius_x*2 + 1);
        Kernel1D<signed_type> kernel_y = FactoryKernel::mean1D<signed_type>(radius_y*2 + 1);
        Gray<E> tmp(input.width, input.height);
        output.reshape(input.width, input.height);
        ConvolveNormalizedNaive::horizontal(kernel_x, input, tmp);
        ConvolveNormalizedNaive::vertical(kernel_y, tmp, output);
    }

    /// Inputs are uniform from 0 to 'max_value', which is also the tolerance's scale. The mean is a running sum of
    /// raw pixel values, so floating point rounding error grows with their range
    template<class E>
    void check_mean( uint32_t width, uint32_t height, uint32_t radius_x, uint32_t radius_y, double max_value = 50 ) {
        std::mt19937 gen(0xBEEF);
        Gray<E> input(width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)max_value, gen);

        Gray<E> expected(1, 1), found(width, height), storage(width, height);
        naive_mean(input, radius_x, radius_y, expected);
        if (radius_x == radius_y)
            tested::BlurImageOps::mean(input, found, radius_x, storage);
        else
            tested::BlurImageOps::mean(input, found, radius_x, radius_y, storage);
        check_within(expected, found, float_tolerance<E>(), max_value);
    }

    /// The output is the input
    template<class E>
    void check_mean_in_place( uint32_t radius_x, uint32_t radius_y ) {
        std::mt19937 gen(0xBEEF);
        Gray<E> image(31, 23);
        ImageMiscOps::fill_uniform(image, (E)0, (E)50, gen);

        Gray<E> expected(1, 1), storage(1, 1);
        naive_mean(image, radius_x, radius_y, expected);
        tested::BlurImageOps::mean(image, image, radius_x, radius_y, storage);
        check_within(expected, image, float_tolerance<E>(), 50);
    }

    /// The same radius along x and y on small images, and different radii including ones larger than the image.
    /// Floats also get inputs which are hard for floating point: long running sums and large values
    template<class E>
    void check_mean_all() {
        for (uint32_t i = 0; i < 3; i++) {
            for (uint32_t radius : {1u, 2u, 5u})
                check_mean<E>(3 + i, 4 + i, radius, radius);
        }
        for (uint32_t size : {4u, 9u, 30u}) {
            check_mean<E>(size + 3, size, 1, 2);
            check_mean<E>(size + 3, size, 4, 1);
            check_mean<E>(size + 3, size, 3, 3);
            check_mean<E>(size + 3, size, 12, 5);
        }
        check_mean_in_place<E>(2, 3);
        if constexpr (std::is_floating_point<E>::value) {
            for (uint32_t radius : {1u, 30u}) {
                check_mean<E>(2001, 7, radius, 2);
                check_mean<E>(7, 2001, 2, radius);
            }
            check_mean<E>(203, 101, 3, 4, 1e5);
        }
    }
}

#endif
