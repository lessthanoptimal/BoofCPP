#ifndef BOOFCPP_BLUR_MEAN_CHECKS_H
#define BOOFCPP_BLUR_MEAN_CHECKS_H

/// Checks of tested::BlurImageOps::mean, without and with an ImageBorder, shared by every variant. Include a variant
/// header, e.g. variant_strict.h, first. Each variant's tests call check_mean_all() and check_mean_with_border_all()
/// for the image types it supports. Results must be within tolerance of naive_mean() or naive_mean_integer()

#include "boofcv/convolve_naive.h"
#include "boofcv/image_border.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>
#include <vector>

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
    void check_mean( std::mt19937& rng, uint32_t width, uint32_t height, uint32_t radius_x, uint32_t radius_y,
                     double max_value = 50 ) {
        Gray<E> input(width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)max_value, rng);

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
    void check_mean_in_place( std::mt19937& rng, uint32_t radius_x, uint32_t radius_y ) {
        Gray<E> image(31, 23);
        ImageMiscOps::fill_uniform(image, (E)0, (E)50, rng);

        Gray<E> expected(1, 1), storage(1, 1);
        naive_mean(image, radius_x, radius_y, expected);
        tested::BlurImageOps::mean(image, image, radius_x, radius_y, storage);
        check_within(expected, image, float_tolerance<E>(), 50);
    }

    /// A radius of zero is rejected
    template<class E>
    void check_mean_bad_radius() {
        Gray<E> input(10, 8), output(10, 8), storage(1, 1);
        EXPECT_THROW(tested::BlurImageOps::mean(input, output, 0, 2, storage), std::invalid_argument);
        EXPECT_THROW(tested::BlurImageOps::mean(input, output, 2, 0, storage), std::invalid_argument);
    }

    /// The same radius along x and y on small images, and different radii including ones larger than the image.
    /// Floats also get inputs which are hard for floating point: long running sums and large values
    template<class E>
    void check_mean_all( std::mt19937& rng ) {
        for (uint32_t i = 0; i < 3; i++) {
            for (uint32_t radius : {1u, 2u, 5u})
                check_mean<E>(rng, 3 + i, 4 + i, radius, radius);
        }
        for (uint32_t size : {4u, 9u, 30u}) {
            check_mean<E>(rng, size + 3, size, 1, 2);
            check_mean<E>(rng, size + 3, size, 4, 1);
            check_mean<E>(rng, size + 3, size, 3, 3);
            check_mean<E>(rng, size + 3, size, 12, 5);
        }
        check_mean_in_place<E>(rng, 2, 3);
        if constexpr (std::is_floating_point<E>::value) {
            for (uint32_t radius : {1u, 30u}) {
                check_mean<E>(rng, 2001, 7, radius, 2);
                check_mean<E>(rng, 7, 2001, 2, radius);
            }
            check_mean<E>(rng, 203, 101, 3, 4, 1e5);
        }
    }

    /// Brute force mean of the region around (x,y), reading pixels through the border, in double
    template<class E>
    double naive_mean( const ImageBorder<E>& border, int32_t x, int32_t y, int32_t rx, int32_t ry ) {
        double sum = 0;
        for (int32_t j = -ry; j <= ry; j++) {
            for (int32_t i = -rx; i <= rx; i++)
                sum += border.get(x + i, y + j);
        }
        return sum/((2*rx + 1)*(2*ry + 1));
    }

    /// Integer division rounded the way BoofCV does it: (total + divisor/2)/divisor with truncation toward zero
    inline int64_t boofcv_round_div( int64_t total, int64_t divisor ) {
        return (total + divisor/2)/divisor;
    }

    /// Integer mean computed directly from BoofCV's definition: a rounded horizontal mean through the border, then a
    /// rounded vertical mean of that through the border
    template<class E>
    Gray<E> naive_mean_integer( const Gray<E>& input, BorderType type, int32_t rx, int32_t ry ) {
        Gray<E> horizontal(input.width, input.height), output(input.width, input.height);
        auto border = FactoryImageBorder::create_SB<E>(type);
        border->setImage(input);
        for (int32_t y = 0; y < (int32_t)input.height; y++) {
            for (int32_t x = 0; x < (int32_t)input.width; x++) {
                int64_t sum = 0;
                for (int32_t i = -rx; i <= rx; i++)
                    sum += border->get(x + i, y);
                horizontal.at(x, y) = (E)boofcv_round_div(sum, 2*rx + 1);
            }
        }
        border->setImage(horizontal);
        for (int32_t y = 0; y < (int32_t)input.height; y++) {
            for (int32_t x = 0; x < (int32_t)input.width; x++) {
                int64_t sum = 0;
                for (int32_t j = -ry; j <= ry; j++)
                    sum += border->get(x, y + j);
                output.at(x, y) = (E)boofcv_round_div(sum, 2*ry + 1);
            }
        }
        return output;
    }

    /// Compares mean() with a border against the brute force mean for every border type, with and without a
    /// sub-image. Inputs are uniform in [min_value, max_value].
    ///
    /// Tolerance scale: the mean is a running sum of pixel values, so its rounding error grows with the largest
    /// magnitude, max(|min_value|, |max_value|)
    template<class E>
    void check_mean_with_border( std::mt19937& rng, uint32_t width, uint32_t height, uint32_t rx, uint32_t ry,
                                 double min_value, double max_value ) {
        Gray<E> input(width, height);
        ImageMiscOps::fill_uniform(input, (E)min_value, (E)max_value, rng);
        const double tol = float_tolerance<E>()*std::max(std::abs(min_value), std::abs(max_value));

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<E> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<E>(type);
                Gray<E> output(1, 1), storage(1, 1);
                std::vector<typename TypeInfo<E>::sum_type> totals;
                tested::BlurImageOps::mean(in, output, rx, ry, *border, storage, totals);

                ASSERT_EQ(width, output.width);
                ASSERT_EQ(height, output.height);
                if constexpr (std::is_floating_point<E>::value) {
                    auto expected_border = FactoryImageBorder::create_SB<E>(type);
                    expected_border->setImage(input);
                    for (int32_t y = 0; y < (int32_t)height; y++) {
                        for (int32_t x = 0; x < (int32_t)width; x++) {
                            ASSERT_NEAR(naive_mean(*expected_border, x, y, rx, ry), output.at(x, y), tol)
                                    << width << "x" << height << " r=" << rx << "," << ry << " border=" << type
                                    << " sub=" << sub << " at " << x << "," << y;
                        }
                    }
                } else {
                    Gray<E> expected = naive_mean_integer(input, type, rx, ry);
                    for (int32_t y = 0; y < (int32_t)height; y++) {
                        for (int32_t x = 0; x < (int32_t)width; x++) {
                            ASSERT_EQ((int64_t)expected.at(x, y), (int64_t)output.at(x, y))
                                    << width << "x" << height << " r=" << rx << "," << ry << " border=" << type
                                    << " sub=" << sub << " at " << x << "," << y;
                        }
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }

    /// The output is the input
    template<class E>
    void check_mean_with_border_in_place( std::mt19937& rng, double max_value ) {
        Gray<E> image(31, 23), output(1, 1), storage(1, 1);
        ImageMiscOps::fill_uniform(image, (E)0, (E)max_value, rng);
        std::vector<typename TypeInfo<E>::sum_type> totals;
        auto border = FactoryImageBorder::create_SB<E>(REFLECT);
        tested::BlurImageOps::mean(image, output, 2, 3, *border, storage, totals);
        tested::BlurImageOps::mean(image, image, 2, 3, *border, storage, totals);
        check_within(output, image, 0.0);
    }

    /// The inner image, the border, and images smaller than the region. Floats also get inputs which are hard for
    /// floating point: cancellation, long running sums and large values
    template<class E>
    void check_mean_with_border_all( std::mt19937& rng, double min_value, double max_value ) {
        for (auto shape : {std::pair{30u, 25u}, {8u, 9u}, {5u, 3u}, {2u, 7u}, {1u, 1u}}) {
            check_mean_with_border<E>(rng, shape.first, shape.second, 1, 1, min_value, max_value);
            check_mean_with_border<E>(rng, shape.first, shape.second, 3, 2, min_value, max_value);
            check_mean_with_border<E>(rng, shape.first, shape.second, 2, 4, min_value, max_value);
        }
        check_mean_with_border_in_place<E>(rng, max_value);
        if constexpr (std::is_floating_point<E>::value) {
            check_mean_with_border<E>(rng, 31, 29, 3, 2, -1e4, 1e4);
            for (uint32_t radius : {1u, 30u}) {
                check_mean_with_border<E>(rng, 2001, 7, radius, 2, 0, 50);
                check_mean_with_border<E>(rng, 7, 2001, 2, radius, 0, 50);
            }
            check_mean_with_border<E>(rng, 203, 101, 3, 4, 0, 1e5);
        }
    }

    /// A radius of zero is rejected
    template<class E>
    void check_mean_with_border_bad_radius() {
        Gray<E> input(10, 8), output(1, 1), storage(1, 1);
        std::vector<typename TypeInfo<E>::sum_type> totals;
        auto border = FactoryImageBorder::create_SB<E>(REFLECT);
        EXPECT_THROW(tested::BlurImageOps::mean(input, output, 0, 2, *border, storage, totals),
                     std::invalid_argument);
        EXPECT_THROW(tested::BlurImageOps::mean(input, output, 2, 0, *border, storage, totals),
                     std::invalid_argument);
    }
}

#endif
