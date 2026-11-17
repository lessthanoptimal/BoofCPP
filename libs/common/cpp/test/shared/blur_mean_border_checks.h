#ifndef BOOFCPP_BLUR_MEAN_BORDER_CHECKS_H
#define BOOFCPP_BLUR_MEAN_BORDER_CHECKS_H

/// Checks of tested::BlurImageOps::mean_border, shared by every variant. Include a variant header, e.g.
/// variant_strict.h, first. Floats must be within floating point tolerance of the mean computed in double, integers
/// must exactly match BoofCV's rounded two pass mean

#include "boofcv/image_border.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>
#include <vector>

namespace blur_mean_border_checks {
    using namespace boofcv;

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

    /// Compares mean_border() against the brute force mean for every border type, with and without a sub-image.
    /// Inputs are uniform in [min_value, max_value].
    ///
    /// Tolerance scale: the mean is a running sum of pixel values, so its rounding error grows with the largest
    /// magnitude, max(|min_value|, |max_value|)
    template<class E>
    void check( uint32_t width, uint32_t height, int32_t rx, int32_t ry, double min_value, double max_value ) {
        Gray<E> input(width, height);
        std::mt19937 gen(0xBEEF + width*31 + height);
        ImageMiscOps::fill_uniform(input, (E)min_value, (E)max_value, gen);
        const double tol = float_tolerance<E>()*std::max(std::abs(min_value), std::abs(max_value));

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<E> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<E>(type);
                Gray<E> output(1, 1), storage(1, 1);
                std::vector<typename TypeInfo<E>::sum_type> totals;
                tested::BlurImageOps::mean_border(in, output, rx, ry, *border, storage, totals);

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
    void check_in_place( double max_value ) {
        Gray<E> image(31, 23), output(1, 1), storage(1, 1);
        std::mt19937 gen(0xBEEF);
        ImageMiscOps::fill_uniform(image, (E)0, (E)max_value, gen);
        std::vector<typename TypeInfo<E>::sum_type> totals;
        auto border = FactoryImageBorder::create_SB<E>(REFLECT);
        tested::BlurImageOps::mean_border(image, output, 2, 3, *border, storage, totals);
        tested::BlurImageOps::mean_border(image, image, 2, 3, *border, storage, totals);
        check_within(output, image, 0.0);
    }

    /// The inner image, the border, and images smaller than the region. Floats also get inputs which are hard for
    /// floating point: cancellation, long running sums and large values
    template<class E>
    void check_all( double min_value, double max_value ) {
        for (auto shape : {std::pair{30u, 25u}, {8u, 9u}, {5u, 3u}, {2u, 7u}, {1u, 1u}}) {
            check<E>(shape.first, shape.second, 1, 1, min_value, max_value);
            check<E>(shape.first, shape.second, 3, 2, min_value, max_value);
            check<E>(shape.first, shape.second, 2, 4, min_value, max_value);
        }
        check_in_place<E>(max_value);
        if constexpr (std::is_floating_point<E>::value) {
            check<E>(31, 29, 3, 2, -1e4, 1e4);
            for (int32_t radius : {1, 30}) {
                check<E>(2001, 7, radius, 2, 0, 50);
                check<E>(7, 2001, 2, radius, 0, 50);
            }
            check<E>(203, 101, 3, 4, 0, 1e5);
        }
    }

    template<class E>
    void check_bad_radius() {
        Gray<E> input(10, 8), output(1, 1), storage(1, 1);
        std::vector<typename TypeInfo<E>::sum_type> totals;
        auto border = FactoryImageBorder::create_SB<E>(REFLECT);
        EXPECT_THROW(tested::BlurImageOps::mean_border(input, output, 0, 2, *border, storage, totals),
                     std::invalid_argument);
        EXPECT_THROW(tested::BlurImageOps::mean_border(input, output, 2, 0, *border, storage, totals),
                     std::invalid_argument);
    }
}

#endif
