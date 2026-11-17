#ifndef BOOFCPP_CONVOLVE_NORMALIZED_CHECKS_H
#define BOOFCPP_CONVOLVE_NORMALIZED_CHECKS_H

/// Checks of tested::ConvolveNormalized, shared by every variant. Include a variant header, e.g. variant_strict.h,
/// first. Results must be within floating point tolerance of the naive normalized convolution in convolve_naive.h, and identical for
/// integers

#include "boofcv/convolve_naive.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>

namespace convolve_normalized_checks {
    using namespace boofcv;

    enum class Op { HORIZONTAL, VERTICAL, CONVOLVE_1D };

    /// Compares one operation against the naive normalized convolution. Pixels are uniform in [lo, hi] and kernel
    /// weights in [1, max_weight], then for floats divided by their sum if 'unit_sum'. A kernel whose sum is one isn't
    /// normalized again. The input is a sub-image, so offset and stride are exercised.
    ///
    /// Tolerance scale: weights are positive and normalized, so each output is a weighted average and its rounding
    /// error grows with max(|pixel|)
    template<class E>
    void check( Op op, uint32_t width, uint32_t height, uint32_t kernel_width, uint32_t offset, double lo,
                double hi, double max_weight, bool unit_sum = false ) {
        typedef typename TypeInfo<E>::signed_type K;
        std::mt19937 gen(0xBEEF + kernel_width*31 + offset*7 + width);

        Gray<E> parent(width + 4, height + 6);
        Gray<E> input = parent.makeSubimage(2, 3, 2 + width, 3 + height);
        ImageMiscOps::fill_uniform(input, (E)lo, (E)hi, gen);

        Kernel1D<K> kernel(kernel_width, offset);
        KernelOps::fill_uniform(kernel, (K)1, (K)max_weight, gen);
        if constexpr (std::is_floating_point<E>::value) {
            if (unit_sum) {
                K sum = 0;
                for (uint32_t i = 0; i < kernel_width; i++)
                    sum += kernel.data[i];
                for (uint32_t i = 0; i < kernel_width; i++)
                    kernel.data[i] /= sum;
            }
        }

        Gray<E> expected(width, height), found(width, height), workspace(width, height);
        if (op == Op::HORIZONTAL) {
            ConvolveNormalizedNaive::horizontal(kernel, input, expected);
            tested::ConvolveNormalized::horizontal(kernel, input, found);
        } else if (op == Op::VERTICAL) {
            ConvolveNormalizedNaive::vertical(kernel, input, expected);
            tested::ConvolveNormalized::vertical(kernel, input, found);
        } else {
            Gray<E> tmp(width, height);
            ConvolveNormalizedNaive::horizontal(kernel, input, tmp);
            ConvolveNormalizedNaive::vertical(kernel, tmp, expected);
            tested::ConvolveNormalized::convolve(kernel, input, found, workspace);
        }
        check_within(expected, found, float_tolerance<E>(), std::max(std::abs(lo), std::abs(hi)));
    }

    /// Kernels smaller than, equal to and larger than the image, and wider than 13. Weights from 1 to 10 make the
    /// kernel's sum differ from one, so it's normalized. Floats also get kernels that sum to one, and inputs which are
    /// hard for floating point: cancellation, long rows and columns, wide kernels and large values
    template<class E>
    void check_all() {
        const Op ops[] = {Op::HORIZONTAL, Op::VERTICAL, Op::CONVOLVE_1D};
        for (Op op : ops) {
            for (uint32_t offset : {3u, 1u, 6u})
                check<E>(op, 5, 6, 7, offset, 0, 50, 10);
            check<E>(op, 7, 7, 7, 3, 0, 50, 10);
            for (uint32_t i = 0; i < 2; i++) {
                check<E>(op, 15 + i, 20 + i, 3, 1, 0, 50, 10);
                check<E>(op, 15 + i, 20 + i, 5, 2, 0, 50, 10);
                check<E>(op, 15 + i, 20 + i, 7, 3, 0, 50, 10);
                check<E>(op, 15 + i, 20 + i, 7, 1, 0, 50, 10);
                check<E>(op, 15 + i, 20 + i, 7, 4, 0, 50, 10);
            }
            check<E>(op, 40, 37, 15, 7, 0, 50, 10);
            if constexpr (std::is_floating_point<E>::value) {
                for (uint32_t offset : {3u, 1u})
                    check<E>(op, 5, 6, 7, offset, 0, 50, 10, true);
                check<E>(op, 15, 20, 5, 2, 0, 50, 10, true);
                check<E>(op, 15, 20, 7, 1, 0, 50, 10, true);
                check<E>(op, 40, 37, 15, 7, 0, 50, 10, true);
                check<E>(op, 31, 29, 7, 3, -50, 50, 10);
                check<E>(op, 40, 37, 9, 4, 0, 1e5, 10);
            }
        }
        if constexpr (std::is_floating_point<E>::value) {
            check<E>(Op::HORIZONTAL, 2001, 7, 31, 15, -50, 50, 10);
            check<E>(Op::VERTICAL, 7, 2001, 31, 15, -50, 50, 10);
        }
    }
}

#endif
