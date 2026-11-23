#ifndef BOOFCPP_BLUR_GAUSSIAN_CHECKS_H
#define BOOFCPP_BLUR_GAUSSIAN_CHECKS_H

/// Checks of tested::BlurImageOps::gaussian, shared by every variant. Include a variant header, e.g.
/// variant_strict.h, first. Results must be within floating point tolerance of the naive normalized convolution in convolve_naive.h
/// along x and then y, and identical for integers

#include "boofcv/convolve_naive.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>

namespace blur_gaussian_checks {
    using namespace boofcv;

    /// A sigma <= 0 is computed from the width, and a width <= 0 from the sigma. Pixels are uniform in [lo, hi] and
    /// the input is a sub-image.
    ///
    /// Tolerance scale: the Gaussian is a weighted average, so its rounding error grows with max(|pixel|)
    template<class E>
    void check( std::mt19937& rng, uint32_t width, uint32_t height, double sigma_x, int32_t width_x, double sigma_y,
                int32_t width_y, double lo, double hi ) {
        typedef typename TypeInfo<E>::signed_type K;
        Gray<E> parent(width + 4, height + 6);
        Gray<E> input = parent.makeSubimage(2, 3, 2 + width, 3 + height);
        ImageMiscOps::fill_uniform(input, (E)lo, (E)hi, rng);

        Kernel1D<K> kernel_x = FactoryKernel::gaussian1D<K>(sigma_x, width_x);
        Kernel1D<K> kernel_y = FactoryKernel::gaussian1D<K>(sigma_y, width_y);
        Gray<E> tmp(width, height), expected(width, height), found(1, 1), storage(1, 1);
        ConvolveNormalizedNaive::horizontal(kernel_x, input, tmp);
        ConvolveNormalizedNaive::vertical(kernel_y, tmp, expected);
        tested::BlurImageOps::gaussian(input, found, sigma_x, width_x, sigma_y, width_y, storage);
        check_within(expected, found, float_tolerance<E>(), std::max(std::abs(lo), std::abs(hi)));
    }

    /// Kernels larger than the image, different kernels along x and y. Floats also get inputs which are hard for
    /// floating point: cancellation, long rows and columns, and large values
    template<class E>
    void check_all( std::mt19937& rng ) {
        for (uint32_t i = 0; i < 3; i++) {
            for (int32_t w : {3, 5, 7, 4})
                check<E>(rng, 3 + i, 4 + i, -1, w, -1, w, 0, 50);
        }
        for (uint32_t size : {4u, 9u, 30u}) {
            check<E>(rng, size + 3, size, -1, 3, -1, 7, 0, 50);
            check<E>(rng, size + 3, size, 1.5, -1, 0.8, -1, 0, 50);
            check<E>(rng, size + 3, size, 2.0, 9, -1, 5, 0, 50);
            check<E>(rng, size + 3, size, -1, 25, 1.0, 3, 0, 50);
        }
        if constexpr (std::is_floating_point<E>::value) {
            check<E>(rng, 31, 29, -1, 7, -1, 9, -50, 50);
            check<E>(rng, 2001, 7, -1, 31, -1, 5, -50, 50);
            check<E>(rng, 7, 2001, -1, 5, -1, 31, -50, 50);
            check<E>(rng, 40, 37, -1, 9, -1, 9, 0, 1e5);
        }
    }
}

#endif
