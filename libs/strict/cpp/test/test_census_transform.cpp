#include "gtest/gtest.h"
#include "boofcv/census_transform.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <random>

using namespace std;
using namespace boofcv;

namespace {
    /// Brute force census of one pixel. Neighbors in row-major order, skipping the center
    int64_t naive_census( const ImageBorder<U8>& border, int32_t cx, int32_t cy, int32_t radius ) {
        const int32_t center = border.get(cx, cy);
        int64_t census = 0;
        int32_t bit = 0;
        for (int32_t y = -radius; y <= radius; y++) {
            for (int32_t x = -radius; x <= radius; x++) {
                if (x == 0 && y == 0)
                    continue;
                if (border.get(cx + x, cy + y) > center)
                    census |= (int64_t)1 << bit;
                bit++;
            }
        }
        return census;
    }

    template<class C>
    void transform( const Gray<U8>& input, Gray<C>& output, ImageBorder<U8>* border ) {
        if constexpr (std::is_same<C, U8>::value)
            CensusTransform::dense3x3(input, output, border);
        else
            CensusTransform::dense5x5(input, output, border);
    }

    /// Compares every pixel against the brute force census, for every border type and a sub-image
    template<class C>
    void compare_to_naive( uint32_t width, uint32_t height, int32_t radius ) {
        Gray<U8> input(width, height);
        std::mt19937 gen(0xBEEF + width*31 + height);
        // small range so there are many equal pixels, which must not set a bit
        ImageMiscOps::fill_uniform(input, (U8)0, (U8)4, gen);

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<U8> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<U8>(type);
                Gray<C> output(1, 1);
                transform(in, output, border.get());

                auto expected_border = FactoryImageBorder::create_SB<U8>(type);
                expected_border->setImage(input);
                ASSERT_EQ(width, output.width);
                ASSERT_EQ(height, output.height);
                for (int32_t y = 0; y < (int32_t)height; y++) {
                    for (int32_t x = 0; x < (int32_t)width; x++) {
                        ASSERT_EQ(naive_census(*expected_border, x, y, radius), (int64_t)output.at(x, y))
                                << width << "x" << height << " border=" << type << " sub=" << sub
                                << " at " << x << "," << y;
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }
}

namespace {
    /// Brute force census of one pixel from sample points. Bit i is samples[i]
    int64_t naive_sample( const ImageBorder<U8>& border, int32_t cx, int32_t cy,
                          const std::vector<Point2D<int32_t>>& samples ) {
        const int32_t center = border.get(cx, cy);
        uint64_t census = 0;
        for (size_t i = 0; i < samples.size(); i++) {
            if (border.get(cx + samples[i].x, cy + samples[i].y) > center)
                census |= (uint64_t)1 << i;
        }
        return (int64_t)census;
    }

    /// Compares every pixel of sample_S64 against the brute force census, for every border type and a sub-image
    void sample_compare_to_naive( const std::vector<Point2D<int32_t>>& samples, uint32_t width, uint32_t height ) {
        Gray<U8> input(width, height);
        std::mt19937 gen(0xBEEF + width*31 + height);
        // small range so there are many equal pixels, which must not set a bit
        ImageMiscOps::fill_uniform(input, (U8)0, (U8)4, gen);

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<U8> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<U8>(type);
                Gray<S64> output(1, 1);
                CensusTransform::sample_S64(in, samples, output, border.get());

                auto expected_border = FactoryImageBorder::create_SB<U8>(type);
                expected_border->setImage(input);
                ASSERT_EQ(width, output.width);
                ASSERT_EQ(height, output.height);
                for (int32_t y = 0; y < (int32_t)height; y++) {
                    for (int32_t x = 0; x < (int32_t)width; x++) {
                        ASSERT_EQ(naive_sample(*expected_border, x, y, samples), output.at(x, y))
                                << width << "x" << height << " border=" << type << " sub=" << sub
                                << " at " << x << "," << y;
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }
}

class CensusTransformTest : public boofcv::BoofCPPTest {};

TEST_F(CensusTransformTest, census_bits_same_as_java) {
    EXPECT_EQ(8u, census_bits(CensusVariants::BLOCK_3_3));
    EXPECT_EQ(24u, census_bits(CensusVariants::BLOCK_5_5));
    EXPECT_EQ(48u, census_bits(CensusVariants::BLOCK_7_7));
    EXPECT_EQ(62u, census_bits(CensusVariants::BLOCK_9_7));
    EXPECT_EQ(64u, census_bits(CensusVariants::BLOCK_13_5));
    EXPECT_EQ(56u, census_bits(CensusVariants::CIRCLE_9));
}

TEST_F(CensusTransformTest, hamming) {
    EXPECT_EQ(0, hamming(0u));
    EXPECT_EQ(1, hamming(0x80000000u));
    EXPECT_EQ(24, hamming(0xFFFFFFu));
    EXPECT_EQ(32, hamming(0xFFFFFFFFu));
    std::mt19937 gen(0xBEEF);
    for (int i = 0; i < 1000; i++) {
        uint32_t v = gen();
        int32_t expected = 0;
        for (int b = 0; b < 32; b++)
            expected += (v >> b) & 1;
        ASSERT_EQ(expected, hamming(v));
    }
}

TEST_F(CensusTransformTest, hamming_64) {
    EXPECT_EQ(0, hamming((uint64_t)0));
    EXPECT_EQ(1, hamming((uint64_t)1 << 63));
    EXPECT_EQ(64, hamming(~(uint64_t)0));
    std::mt19937_64 gen(0xBEEF);
    for (int i = 0; i < 1000; i++) {
        uint64_t v = gen();
        int32_t expected = 0;
        for (int b = 0; b < 64; b++)
            expected += (v >> b) & 1;
        ASSERT_EQ(expected, hamming(v));
    }
}

/// Same points in the same order as BoofCV's createBlockSamples()
TEST_F(CensusTransformTest, create_block_samples) {
    auto samples = CensusTransform::create_block_samples(2, 1);
    std::vector<std::pair<int32_t, int32_t>> expected = {
            {-2, -1}, {-1, -1}, {0, -1}, {1, -1}, {2, -1},
            {-2, 0}, {-1, 0}, {1, 0}, {2, 0},
            {-2, 1}, {-1, 1}, {0, 1}, {1, 1}, {2, 1}};
    ASSERT_EQ(expected.size(), samples.size());
    for (size_t i = 0; i < expected.size(); i++) {
        EXPECT_EQ(expected[i].first, samples[i].x) << i;
        EXPECT_EQ(expected[i].second, samples[i].y) << i;
    }
}

/// Same points in the same order as BoofCV's createCircleSamples(), which goes down each column
TEST_F(CensusTransformTest, create_circle_samples) {
    auto samples = CensusTransform::create_circle_samples();
    ASSERT_EQ(56u, samples.size());
    // number of points in each column, from x = -4 to 4
    const int32_t column_size[] = {3, 5, 7, 9, 8, 9, 7, 5, 3};
    size_t i = 0;
    for (int32_t x = -4; x <= 4; x++) {
        const int32_t n = column_size[x + 4];
        // y increases down the column, which is centered and skips the center pixel
        int32_t y = -(n + (x == 0 ? 1 : 0))/2;
        for (int32_t j = 0; j < n; j++, i++, y++) {
            if (x == 0 && y == 0)
                y++;
            ASSERT_EQ(x, samples[i].x) << i;
            ASSERT_EQ(y, samples[i].y) << i;
        }
    }
}

TEST_F(CensusTransformTest, sample_S64_compare_to_naive) {
    const std::vector<std::vector<Point2D<int32_t>>> all_samples = {
            CensusTransform::create_block_samples(3, 3), CensusTransform::create_block_samples(4, 3),
            CensusTransform::create_block_samples(6, 2), CensusTransform::create_circle_samples()};
    for (const auto& samples : all_samples) {
        // the smallest sizes have no inner image
        for (auto shape : {std::pair{40u, 30u}, {41u, 9u}, {13u, 20u}, {12u, 12u}, {1u, 1u}, {3u, 15u}, {15u, 2u}})
            sample_compare_to_naive(samples, shape.first, shape.second);
    }
}

/// Without a border, only pixels farther than the largest x or y of a sample from every edge are written. With a
/// 13x5 block that's 6 from the top and bottom too, same as BoofCV
TEST_F(CensusTransformTest, sample_S64_no_border) {
    auto samples = CensusTransform::create_block_samples(6, 2);
    Gray<U8> input(20, 16);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
    Gray<S64> output(20, 16);
    ImageMiscOps::fill(output, (S64)-1);
    CensusTransform::sample_S64(input, samples, output, (ImageBorder<U8>*)nullptr);

    auto border = FactoryImageBorder::create_SB<U8>(REFLECT);
    border->setImage(input);
    for (int32_t y = 0; y < 16; y++) {
        for (int32_t x = 0; x < 20; x++) {
            bool inner = x >= 6 && y >= 6 && x < 14 && y < 10;
            int64_t expected = inner ? naive_sample(*border, x, y, samples) : -1;
            ASSERT_EQ(expected, output.at(x, y)) << x << " " << y;
        }
    }
}

TEST_F(CensusTransformTest, sample_S64_too_many_samples) {
    Gray<U8> input(10, 10);
    Gray<S64> output(10, 10);
    auto samples = CensusTransform::create_block_samples(4, 4);
    EXPECT_THROW(CensusTransform::sample_S64(input, samples, output, (ImageBorder<U8>*)nullptr), invalid_argument);
    EXPECT_THROW(FilterCensusTransformSampleS64<U8>(samples, nullptr), invalid_argument);
}

TEST_F(CensusTransformTest, dense3x3_compare_to_naive) {
    // the smallest sizes have no inner image
    for (auto shape : {std::pair{30u, 25u}, {31u, 4u}, {3u, 20u}, {2u, 2u}, {2u, 7u}})
        compare_to_naive<U8>(shape.first, shape.second, 1);
}

TEST_F(CensusTransformTest, dense5x5_compare_to_naive) {
    for (auto shape : {std::pair{30u, 25u}, {31u, 6u}, {5u, 20u}, {3u, 3u}, {4u, 9u}})
        compare_to_naive<S32>(shape.first, shape.second, 2);
}

/// Without a border, only the inner image is written
TEST_F(CensusTransformTest, no_border) {
    Gray<U8> input(12, 10);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
    Gray<S32> output(12, 10);
    ImageMiscOps::fill(output, -1);
    CensusTransform::dense5x5(input, output, (ImageBorder<U8>*)nullptr);

    auto border = FactoryImageBorder::create_SB<U8>(REFLECT);
    border->setImage(input);
    for (int32_t y = 0; y < 10; y++) {
        for (int32_t x = 0; x < 12; x++) {
            bool inner = x >= 2 && y >= 2 && x < 10 && y < 8;
            int64_t expected = inner ? naive_census(*border, x, y, 2) : -1;
            ASSERT_EQ(expected, (int64_t)output.at(x, y)) << x << " " << y;
        }
    }
}

/// Images smaller than the census region. Every pixel is in the border
TEST_F(CensusTransformTest, smaller_than_region) {
    for (auto shape : {std::pair{1u, 1u}, {1u, 5u}, {5u, 1u}})
        compare_to_naive<U8>(shape.first, shape.second, 1);
    for (auto shape : {std::pair{1u, 1u}, {2u, 5u}, {5u, 2u}, {1u, 7u}})
        compare_to_naive<S32>(shape.first, shape.second, 2);
}

class FilterCensusTransformTest : public boofcv::BoofCPPTest {};

TEST_F(FilterCensusTransformTest, same_as_census_transform) {
    Gray<U8> input(20, 15);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
    auto border = FactoryImageBorder::create_SB<U8>(WRAP);

    Gray<U8> expected3(1, 1), found3(1, 1);
    CensusTransform::dense3x3(input, expected3, border.get());
    FilterCensusTransformD33U8<U8>(FactoryImageBorder::create_SB<U8>(WRAP)).process(input, found3);
    check_equals(expected3, found3, (U8)0);

    Gray<S32> expected5(1, 1), found5(1, 1);
    CensusTransform::dense5x5(input, expected5, border.get());
    FilterCensusTransformD55S32<U8>(FactoryImageBorder::create_SB<U8>(WRAP)).process(input, found5);
    check_equals(expected5, found5, 0);

    auto samples = CensusTransform::create_circle_samples();
    Gray<S64> expected64(1, 1), found64(1, 1);
    CensusTransform::sample_S64(input, samples, expected64, border.get());
    FilterCensusTransformSampleS64<U8>(samples, FactoryImageBorder::create_SB<U8>(WRAP)).process(input, found64);
    check_equals(expected64, found64, (S64)0);
}
