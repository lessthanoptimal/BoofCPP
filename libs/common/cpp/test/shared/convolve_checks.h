#ifndef BOOFCPP_CONVOLVE_CHECKS_H
#define BOOFCPP_CONVOLVE_CHECKS_H

/// Checks of tested::ConvolveImage, shared by every variant. Include a variant header, e.g. variant_strict.h, first.
/// Each variant's tests call check_all() for the image types it supports. Results must be within floating point
/// tolerance of the naive convolution in convolve_naive.h over the whole image, and identical for integers

#include "boofcv/convolve_naive.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <memory>
#include <random>

namespace convolve_checks {
    using namespace boofcv;

    enum class Op { HORIZONTAL, VERTICAL, CONVOLVE };

    template<class E>
    std::shared_ptr<ImageBorder<E>> create_border( int which ) {
        if (which == 0)
            return std::make_shared<ImageBorderValue<E>>((E)5);
        const BorderType types[] = {EXTENDED, REFLECT, WRAP, ZERO};
        return FactoryImageBorder::create_SB<E>(types[which - 1]);
    }
    constexpr int NUM_BORDERS = 5;

    /// Compares one operation against the naive convolution. Pixels are uniform in [lo, hi] and kernel weights in
    /// [-max_weight, max_weight] if 'negative', which makes floats cancel, otherwise in [0, max_weight]. The input is a
    /// sub-image, so offset and stride are exercised.
    ///
    /// Tolerance scale: rounding error of a sum grows with the sum of its terms' magnitudes, at most
    /// sum(|weight|)*max(|pixel|)
    template<class E, class R = E>
    void check( Op op, uint32_t width, uint32_t height, uint32_t kernel_width, uint32_t offset, int border_type,
                double lo, double hi, double max_weight, bool negative ) {
        typedef typename TypeInfo<E>::signed_type K;
        std::mt19937 gen(0xBEEF + kernel_width*31 + offset*7 + border_type);

        Gray<E> parent(width + 4, height + 6);
        Gray<E> input = parent.makeSubimage(2, 3, 2 + width, 3 + height);
        ImageMiscOps::fill_uniform(input, (E)lo, (E)hi, gen);

        std::shared_ptr<ImageBorder<E>> border = create_border<E>(border_type);
        border->setImage(input);

        Gray<R> expected(width, height), found(1, 1);
        double sum_weights = 0;
        const K kernel_lo = negative ? (K)-max_weight : (K)0;
        if (op == Op::CONVOLVE) {
            Kernel2D<K> kernel(kernel_width, offset);
            KernelOps::fill_uniform(kernel, kernel_lo, (K)max_weight, gen);
            for (uint32_t i = 0; i < kernel_width*kernel_width; i++)
                sum_weights += std::abs((double)kernel.data[i]);
            ConvolveNaive::convolve(kernel, *border, expected);
            tested::ConvolveImage::convolve(kernel, *border, found);
        } else {
            Kernel1D<K> kernel(kernel_width, offset);
            KernelOps::fill_uniform(kernel, kernel_lo, (K)max_weight, gen);
            for (uint32_t i = 0; i < kernel_width; i++)
                sum_weights += std::abs((double)kernel.data[i]);
            if (op == Op::HORIZONTAL) {
                ConvolveNaive::horizontal(kernel, *border, expected);
                tested::ConvolveImage::horizontal(kernel, *border, found);
            } else {
                ConvolveNaive::vertical(kernel, *border, expected);
                tested::ConvolveImage::vertical(kernel, *border, found);
            }
        }
        double max_pixel = std::max(std::abs(lo), std::abs(hi));
        check_within(expected, found, float_tolerance<R>(), sum_weights*max_pixel);
    }

    /// Every operation and border with kernels smaller than, equal to and larger than the image. Floats also get
    /// inputs which are hard for floating point: cancellation, long rows and columns, wide kernels and large values
    template<class E, class R = E>
    void check_all() {
        const Op ops[] = {Op::HORIZONTAL, Op::VERTICAL, Op::CONVOLVE};
        constexpr bool is_float = std::is_floating_point<E>::value;
        for (Op op : ops) {
            for (int border = 0; border < NUM_BORDERS; border++) {
                // Kernel larger than the image
                for (uint32_t offset : {3u, 1u, 6u})
                    check<E, R>(op, 5, 6, 7, offset, border, 0, 50, 10, false);
                // Kernel the size of the image
                check<E, R>(op, 7, 7, 7, 3, border, 0, 50, 10, false);
                for (uint32_t i = 0; i < 2; i++) {
                    check<E, R>(op, 15 + i, 20 + i, 3, 1, border, 0, 50, 10, false);
                    check<E, R>(op, 15 + i, 20 + i, 5, 2, border, 0, 50, 10, false);
                    check<E, R>(op, 15 + i, 20 + i, 7, 1, border, 0, 50, 10, false);
                    check<E, R>(op, 15 + i, 20 + i, 7, 4, border, 0, 50, 10, true);
                }
                if constexpr (is_float) {
                    check<E, R>(op, 31, 29, 7, 3, border, -50, 50, 10, true);
                    check<E, R>(op, 40, 37, 9, 4, border, -1e5, 1e5, 1, true);
                }
            }
        }
        if constexpr (is_float) {
            for (int border = 0; border < NUM_BORDERS; border++) {
                check<E, R>(Op::HORIZONTAL, 2001, 7, 31, 15, border, -50, 50, 1, true);
                check<E, R>(Op::VERTICAL, 7, 2001, 31, 15, border, -50, 50, 1, true);
                check<E, R>(Op::CONVOLVE, 101, 97, 21, 10, border, -50, 50, 1, true);
            }
        }
    }
}

#endif
