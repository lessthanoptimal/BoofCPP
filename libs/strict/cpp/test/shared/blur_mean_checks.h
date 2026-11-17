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

    /// Largest input value. It's the tolerance's scale because mean is a running sum of raw pixel values, so F32's
    /// rounding error grows with their range
    constexpr double max_value = 50;

    template<class E>
    void check_mean( uint32_t width, uint32_t height, uint32_t radius_x, uint32_t radius_y ) {
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

    /// The same radius along x and y on small images, and different radii including ones larger than the image
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
    }
}

#endif
