#include "gtest/gtest.h"
#include "boofcv/census_transform.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"
#include "variant_strict.h"
#include "shared/census_checks.h"

#include <random>

using namespace std;
using namespace boofcv;

class CensusTransformTest : public boofcv::BoofCPPTest {};

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

TEST_F(CensusTransformTest, U8) { census_checks::check_all<U8>(); }
TEST_F(CensusTransformTest, U16) { census_checks::check_all<U16>(); }
TEST_F(CensusTransformTest, F32) { census_checks::check_all<F32>(); }
TEST_F(CensusTransformTest, too_many_samples) { census_checks::check_too_many_samples<U8>(); }

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

TEST_F(FilterCensusTransformTest, sample_too_many_samples) {
    EXPECT_THROW(FilterCensusTransformSampleS64<U8>(CensusTransform::create_block_samples(4, 4), nullptr),
                 invalid_argument);
}

/// The circle of the shared checks is BoofCV's
TEST_F(CensusTransformTest, shared_circle_is_boofcv) {
    auto expected = CensusTransform::create_circle_samples();
    auto found = census_checks::circle();
    ASSERT_EQ(expected.size(), found.size());
    for (size_t i = 0; i < expected.size(); i++) {
        EXPECT_EQ(expected[i].x, found[i].x);
        EXPECT_EQ(expected[i].y, found[i].y);
    }
}
