#ifndef BOOFCPP_CENSUS_CHECKS_H
#define BOOFCPP_CENSUS_CHECKS_H

/// Checks of tested::CensusTransform, shared by every variant. Include a variant header, e.g. variant_strict.h, first.
/// Each variant's tests call the checks for the image types it supports. Results must be identical to a brute force
/// census of every pixel, read through the border

#include "boofcv/geometry_types.h"
#include "boofcv/image_border.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace census_checks {
    using namespace boofcv;

    enum class Op { DENSE_3X3, DENSE_5X5, SAMPLE_S64 };

    /// Every point of a (2*radius_x+1) by (2*radius_y+1) block except the center, in row-major order. The bit order of
    /// the dense census transforms
    inline std::vector<Point2D<int32_t>> block( int32_t radius_x, int32_t radius_y ) {
        std::vector<Point2D<int32_t>> points;
        for (int32_t y = -radius_y; y <= radius_y; y++) {
            for (int32_t x = -radius_x; x <= radius_x; x++) {
                if (x != 0 || y != 0)
                    points.emplace_back(x, y);
            }
        }
        return points;
    }

    /// The 56 points of BoofCV's CIRCLE_9 within a 9x9 region, in its order: down each column
    inline std::vector<Point2D<int32_t>> circle() {
        std::vector<Point2D<int32_t>> points;
        for (int32_t column = 0; column < 9; column++) {
            const int32_t row0 = column <= 4 ? std::max(0, 3 - column) : column - 5;
            for (int32_t row = row0; row < 9 - row0; row++) {
                if (column != 4 || row != 4)
                    points.emplace_back(column - 4, row - 4);
            }
        }
        return points;
    }

    /// Sampled regions: the 7x7, 9x7, 13x5 and circle regions of BoofCV's CensusVariants, and a sparse one with
    /// points at different distances
    inline std::vector<std::vector<Point2D<int32_t>>> sample_sets() {
        std::vector<Point2D<int32_t>> sparse;
        for (auto p : {std::pair{5, -1}, {0, 2}, {-3, 0}, {1, 1}, {-2, -6}})
            sparse.emplace_back(p.first, p.second);
        return {block(3, 3), block(4, 3), block(6, 2), circle(), sparse};
    }

    /// Brute force census of one pixel. Bit i is set if samples[i] is greater than the center
    template<class E>
    int64_t naive( const ImageBorder<E>& border, int32_t cx, int32_t cy, const std::vector<Point2D<int32_t>>& samples ) {
        const auto center = border.get(cx, cy);
        uint64_t census = 0;
        for (size_t i = 0; i < samples.size(); i++) {
            if (border.get(cx + samples[i].x, cy + samples[i].y) > center)
                census |= (uint64_t)1 << i;
        }
        return (int64_t)census;
    }

    /// Values in [0, 4], so many neighbors are equal and must not set a bit. Floats have a fractional part and a NaN,
    /// which compares false
    template<class E>
    Gray<E> random_image( uint32_t width, uint32_t height, std::mt19937& gen ) {
        Gray<E> image(width, height);
        if constexpr (std::is_floating_point<E>::value) {
            std::uniform_int_distribution<int> value(0, 8);
            for (uint32_t y = 0; y < height; y++)
                for (uint32_t x = 0; x < width; x++)
                    image.at(x, y) = (E)(value(gen)*0.5);
            image.at(width/2, height/2) = std::numeric_limits<E>::quiet_NaN();
        } else {
            ImageMiscOps::fill_uniform(image, (E)0, (E)4, gen);
        }
        return image;
    }

    /// Runs the operation and returns its output as 64-bit values
    template<class E>
    Gray<S64> transform( Op op, const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples,
                         ImageBorder<E>* border, int64_t fill ) {
        Gray<S64> result(input.width, input.height);
        if (op == Op::SAMPLE_S64) {
            ImageMiscOps::fill(result, (S64)fill);
            tested::CensusTransform::sample_S64(input, samples, result, border);
            return result;
        }
        auto copy = [&]( const auto& output ) {
            for (uint32_t y = 0; y < input.height; y++)
                for (uint32_t x = 0; x < input.width; x++)
                    result.at(x, y) = (S64)output.at(x, y);
        };
        if (op == Op::DENSE_3X3) {
            Gray<U8> output(input.width, input.height);
            ImageMiscOps::fill(output, (U8)fill);
            tested::CensusTransform::dense3x3(input, output, border);
            copy(output);
        } else {
            Gray<S32> output(input.width, input.height);
            ImageMiscOps::fill(output, (S32)fill);
            tested::CensusTransform::dense5x5(input, output, border);
            copy(output);
        }
        return result;
    }

    inline std::vector<Point2D<int32_t>> op_samples( Op op, const std::vector<Point2D<int32_t>>& samples ) {
        if (op == Op::DENSE_3X3)
            return block(1, 1);
        if (op == Op::DENSE_5X5)
            return block(2, 2);
        return samples;
    }

    /// Every pixel against the brute force census, for every border type, a whole image and a sub-image
    template<class E>
    void compare( Op op, uint32_t width, uint32_t height, const std::vector<Point2D<int32_t>>& sample_points = {} ) {
        const std::vector<Point2D<int32_t>> samples = op_samples(op, sample_points);
        std::mt19937 gen(0xBEEF + width*31 + height);
        Gray<E> input = random_image<E>(width, height, gen);

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<E> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<E>(type);
                Gray<S64> found = transform(op, in, samples, border.get(), 0);

                auto expected_border = FactoryImageBorder::create_SB<E>(type);
                expected_border->setImage(input);
                for (int32_t y = 0; y < (int32_t)height; y++) {
                    for (int32_t x = 0; x < (int32_t)width; x++) {
                        ASSERT_EQ(naive(*expected_border, x, y, samples), found.at(x, y))
                                << width << "x" << height << " border=" << type << " sub=" << sub
                                << " at " << x << "," << y;
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }

    /// Without a border, only pixels farther than the largest x or y of a sample from every edge are written
    template<class E>
    void no_border( Op op, const std::vector<Point2D<int32_t>>& sample_points = {} ) {
        const std::vector<Point2D<int32_t>> samples = op_samples(op, sample_points);
        int32_t radius = 0;
        for (const auto& p : samples)
            radius = std::max(radius, std::max(std::abs(p.x), std::abs(p.y)));
        const int32_t width = 20 + 2*radius, height = 9 + 2*radius;
        std::mt19937 gen(0xBEEF);
        Gray<E> input = random_image<E>(width, height, gen);
        // pixels which must not be written keep 0x55
        Gray<S64> found = transform(op, input, samples, (ImageBorder<E>*)nullptr, 0x55);

        auto border = FactoryImageBorder::create_SB<E>(REFLECT);
        border->setImage(input);
        for (int32_t y = 0; y < height; y++) {
            for (int32_t x = 0; x < width; x++) {
                bool inner = x >= radius && y >= radius && x < width - radius && y < height - radius;
                int64_t expected = inner ? naive(*border, x, y, samples) : 0x55;
                ASSERT_EQ(expected, found.at(x, y)) << x << " " << y;
            }
        }
    }

    /// Images with and without an inner image, those as small as one pixel, and every sampled region
    template<class E>
    void check_all() {
        for (auto shape : {std::pair{30u, 25u}, {31u, 4u}, {3u, 20u}, {2u, 2u}, {2u, 7u}, {1u, 1u}, {1u, 5u}, {5u, 1u}})
            compare<E>(Op::DENSE_3X3, shape.first, shape.second);
        for (auto shape : {std::pair{30u, 25u}, {31u, 6u}, {5u, 20u}, {3u, 3u}, {4u, 9u}, {1u, 1u}, {2u, 5u}, {5u, 2u},
                           {1u, 7u}})
            compare<E>(Op::DENSE_5X5, shape.first, shape.second);
        for (const auto& samples : sample_sets()) {
            for (auto shape : {std::pair{40u, 30u}, {41u, 9u}, {13u, 20u}, {12u, 12u}, {1u, 1u}, {3u, 15u}, {15u, 2u}})
                compare<E>(Op::SAMPLE_S64, shape.first, shape.second, samples);
            no_border<E>(Op::SAMPLE_S64, samples);
        }
        no_border<E>(Op::DENSE_3X3);
        no_border<E>(Op::DENSE_5X5);
    }

    /// sample_S64 has at most 64 bits
    template<class E>
    void check_too_many_samples() {
        Gray<E> input(10, 10);
        Gray<S64> output(10, 10);
        EXPECT_THROW(tested::CensusTransform::sample_S64(input, block(4, 4), output, (ImageBorder<E>*)nullptr),
                     std::invalid_argument);
    }
}

#endif
