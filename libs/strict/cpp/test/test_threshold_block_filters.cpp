#include "gtest/gtest.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>
#include "boofcv/threshold_block_filters.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"
#include "variant_strict.h"
#include "shared/threshold_checks.h"
#include "shared/threshold_block_reference.h"

using namespace std;
using namespace boofcv;

class DummyThresholdBlockCommon : public ThresholdBlockCommon<U8> {
public:
    explicit DummyThresholdBlockCommon( uint32_t width )
            : ThresholdBlockCommon(ConfigLength::fixed(width), true) {}

protected:
    void begin( uint32_t width ) override {}
    void statistics( const Gray<U8>& input, uint32_t by ) override {}
    void threshold_rows( const Gray<U8>& input, Gray<U8>& output, uint32_t by ) override {}
};

class ThresholdBlockCommonTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockCommonTest, selectBlockSize) {
    uint32_t width = 30;

    DummyThresholdBlockCommon alg(width);

    alg.selectBlockSize(300,330,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(329,301,width);
    ASSERT_EQ(32,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(301,329,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(32,alg.blockHeight);
}

class ThresholdBlockMeanTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMeanTest, standard_tests_U8) {
    threshold_checks::block_all(rng, threshold_checks::block_mean<U8>());
}
TEST_F(ThresholdBlockMeanTest, standard_tests_F32) {
    threshold_checks::block_all(rng, threshold_checks::block_mean<F32>());
}

class ThresholdBlockOtsuTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockOtsuTest, standard_tests_U8) {
    threshold_checks::block_all(rng, threshold_checks::block_otsu<U8>());
}
TEST_F(ThresholdBlockOtsuTest, standard_tests_F32) {
    threshold_checks::block_all(rng, threshold_checks::block_otsu<F32>());
}

class ThresholdBlockMinMaxTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMinMaxTest, standard_tests_U8) {
    threshold_checks::block_all(rng, threshold_checks::block_min_max<U8>());
}
TEST_F(ThresholdBlockMinMaxTest, standard_tests_F32) {
    threshold_checks::block_all(rng, threshold_checks::block_min_max<F32>());
}

namespace reference = threshold_block_reference;

namespace {
    enum class Content { UNIFORM, FLAT, SMOOTH, HARD };

    /// Test images. HARD has values which are hard for floating point: large and tiny magnitudes, negative values and
    /// signed zeros for F32 (when 'limited' is false), and long runs of equal values
    template<class E>
    Gray<E> create_image( uint32_t width, uint32_t height, Content content, bool limited, std::mt19937& rng ) {
        Gray<E> image(width, height);
        std::uniform_real_distribution<double> uniform(0, 255);
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                double v = 0;
                switch (content) {
                    case Content::UNIFORM: v = uniform(rng); break;
                    case Content::FLAT: v = 128; break;
                    case Content::SMOOTH: v = 100 + 60*std::sin(x*0.05)*std::cos(y*0.07) + uniform(rng)/50; break;
                    case Content::HARD: {
                        const uint32_t kind = (x/7 + y/5)%5;
                        if (kind == 0) v = 200;
                        else if (kind == 1) v = uniform(rng);
                        else if (std::is_integral_v<E> || limited) v = kind*50;
                        else if (kind == 2) v = 1e6 + uniform(rng);
                        else if (kind == 3) v = -uniform(rng)*1e-30;
                        else v = (x + y)%2 == 0 ? 0.0 : -0.0;
                        break;
                    }
                }
                image.at(x, y) = std::is_integral_v<E> ? (E)std::lround(v) : (E)v;
            }
        }
        return image;
    }

    const std::vector<std::pair<uint32_t, uint32_t>> SHAPES = {{16, 16}, {37, 29}, {100, 120}, {160, 47}};
    const Content CONTENTS[] = {Content::UNIFORM, Content::FLAT, Content::SMOOTH, Content::HARD};

    /// The filter gives the expected image. Sub-images are checked by the standard tests
    template<class E>
    void check_same( const Gray<U8>& expected, InputToBinary<Gray<E>>& alg, const Gray<E>& input ) {
        Gray<U8> found(input.width, input.height);
        alg.process(input, found);
        check_equals(expected, found);
    }

    /// Calls check(input, width) for every test image and block width that fits it
    template<class E, class Check>
    void for_each_input( std::mt19937& rng, bool limited, Check check ) {
        for (auto shape : SHAPES) {
            for (Content content : CONTENTS) {
                Gray<E> input = create_image<E>(shape.first, shape.second, content, limited, rng);
                for (uint32_t width : {2u, 5u, 14u, 40u}) {
                    if (width > std::min(shape.first, shape.second))
                        continue;
                    check(input, ConfigLength::fixed(width));
                }
            }
        }
    }

    template<class E>
    void block_mean_same_as_reference( std::mt19937& rng ) {
        for_each_input<E>(rng, false, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (double scale : {0.95, 1.0, 1.3, 0.82, -0.5}) {
                    ThresholdBlockMean<E> alg(width, local, scale, true);
                    check_same<E>(reference::mean(input, width, local, scale, true), alg, input);
                }
        });
    }

    template<class E>
    void block_otsu_same_as_reference( std::mt19937& rng ) {
        // F32 pixels must fit in U8, which they're cast to
        for_each_input<E>(rng, true, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (bool otsu2 : {true, false})
                    for (auto tuning_scale : {std::pair<double, double>{0.0, 0.95}, {0.3, 1.3}}) {
                        const double tuning = tuning_scale.first, scale = tuning_scale.second;
                        ThresholdBlockOtsu<E> alg(otsu2, width, tuning, scale, true, local);
                        check_same<E>(reference::otsu(input, otsu2, width, tuning, scale, true, local), alg, input);
                    }
        });
    }

    template<class E>
    void block_min_max_same_as_reference( std::mt19937& rng ) {
        for_each_input<E>(rng, false, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (float spread : {0.0f, 1.0f, 10.0f})
                    for (double scale : {0.95, 1.0, 1.3, 0.82, -0.5}) {
                        ThresholdBlockMinMax<E> alg(spread, width, local, scale, true);
                        check_same<E>(reference::min_max(input, spread, width, local, scale, true), alg, input);
                    }
        });
    }

    /// F32 image with NaN pixels, including the first pixel of some blocks, which is never replaced
    Gray<F32> nan_image( std::mt19937& rng ) {
        Gray<F32> input = create_image<F32>(100, 120, Content::UNIFORM, true, rng);
        for (uint32_t y = 0; y < input.height; y += 3)
            for (uint32_t x = 0; x < input.width; x += 7)
                input.at(x, y) = std::numeric_limits<F32>::quiet_NaN();
        return input;
    }
}

TEST_F(ThresholdBlockMeanTest, same_as_reference_U8) { block_mean_same_as_reference<U8>(rng); }
TEST_F(ThresholdBlockMeanTest, same_as_reference_F32) { block_mean_same_as_reference<F32>(rng); }
TEST_F(ThresholdBlockOtsuTest, same_as_reference_U8) { block_otsu_same_as_reference<U8>(rng); }
TEST_F(ThresholdBlockOtsuTest, same_as_reference_F32) { block_otsu_same_as_reference<F32>(rng); }
TEST_F(ThresholdBlockMinMaxTest, same_as_reference_U8) { block_min_max_same_as_reference<U8>(rng); }
TEST_F(ThresholdBlockMinMaxTest, same_as_reference_F32) { block_min_max_same_as_reference<F32>(rng); }

TEST_F(ThresholdBlockMeanTest, nan_same_as_reference) {
    Gray<F32> input = nan_image(rng);
    for (bool local : {true, false}) {
        ThresholdBlockMean<F32> alg(ConfigLength::fixed(10), local, 1.0, true);
        check_same<F32>(reference::mean(input, ConfigLength::fixed(10), local, 1.0, true), alg, input);
    }
}

TEST_F(ThresholdBlockMinMaxTest, nan_same_as_reference) {
    Gray<F32> input = nan_image(rng);
    for (bool local : {true, false}) {
        for (bool down : {true, false}) {
            ThresholdBlockMinMax<F32> alg(1.0f, ConfigLength::fixed(7), local, 1.0f, down);
            check_same<F32>(reference::min_max(input, 1.0f, ConfigLength::fixed(7), local, 1.0f, down), alg, input);
        }
    }
}

/// A signed image whose scaled averages are below the lowest S16 value, and textureless blocks
TEST_F(ThresholdBlockMinMaxTest, average_outside_signed_range) {
    Gray<S16> input(64, 48);
    ImageMiscOps::fill_uniform(input, (S16)-32768, (S16)-32000, rng);
    for (uint32_t y = 0; y < 16; y++)
        for (uint32_t x = 0; x < 16; x++)
            input.at(x, y) = 100;
    for (bool local : {true, false}) {
        for (bool down : {true, false}) {
            for (float scale : {0.95f, 1.3f}) {
                ThresholdBlockMinMax<S16> alg(1.0f, ConfigLength::fixed(8), local, scale, down);
                check_same<S16>(reference::min_max(input, 1.0f, ConfigLength::fixed(8), local, scale, down), alg,
                                input);
            }
        }
    }
}

/// An image with only negative values, whose region maximum is below zero
TEST_F(ThresholdBlockMinMaxTest, negative_pixels) {
    Gray<F32> input(12, 8);
    for (uint32_t y = 0; y < input.height; y++)
        for (uint32_t x = 0; x < input.width; x++)
            input.at(x, y) = (x + y)%2 == 0 ? -10.0f : -20.0f;
    for (bool down : {true, false}) {
        ThresholdBlockMinMax<F32> alg(1.0f, ConfigLength::fixed(4), true, 1.0f, down);
        Gray<U8> output(input.width, input.height);
        alg.process(input, output);
        // the threshold is (-10 + -20)/2 = -15
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                EXPECT_EQ(down == (input.at(x, y) == -20.0f), output.at(x, y) == 1) << x << " " << y;
        check_same<F32>(reference::min_max(input, 1.0f, ConfigLength::fixed(4), true, 1.0f, down), alg, input);
    }
}

namespace {
    /// Counts the 1s from thresholding up an image of alternating 'a' and 'b'
    uint32_t checkerboard_ones( U8 a, U8 b, double spread ) {
        Gray<U8> input(20, 16);
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                input.at(x, y) = (x + y)%2 == 0 ? a : b;
        ThresholdBlockMinMax<U8> alg(spread, ConfigLength::fixed(5), true, 1.0f, false);
        Gray<U8> output(input.width, input.height);
        alg.process(input, output);
        check_same<U8>(reference::min_max(input, spread, ConfigLength::fixed(5), true, 1.0f, false), alg, input);
        uint32_t ones = 0;
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                ones += output.at(x, y);
        return ones;
    }
}

/// Any negative spread disables the textureless check. Thresholding up, a flat image is then all 0, not all 1
TEST_F(ThresholdBlockMinMaxTest, negative_spread_disables) {
    for (double spread : {-0.1, -1.0})
        EXPECT_EQ(0u, checkerboard_ones(128, 128, spread)) << spread;
    EXPECT_EQ(20u*16u, checkerboard_ones(128, 128, 0.0));
}

/// Integer images round the spread like Java's Math.round, ties up, then cast it to an int
TEST_F(ThresholdBlockMinMaxTest, texture_threshold) {
    auto threshold = []( double minimumSpread ) {
        return ThresholdBlockMinMax<U8>(minimumSpread, ConfigLength::fixed(5), true, 1.0, false).texture_threshold();
    };
    EXPECT_EQ(-1, threshold(-0.1));
    EXPECT_EQ(-1, threshold(-1e20));
    EXPECT_EQ(0, threshold(0));
    EXPECT_EQ(0, threshold(std::nextafter(0.5, 0.0)));
    EXPECT_EQ(1, threshold(0.5));
    EXPECT_EQ(2, threshold(2.4999999999));
    EXPECT_EQ(3, threshold(2.5));
    EXPECT_EQ(3, threshold(2.9999999999));
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), threshold(1e20));
    EXPECT_EQ(0, threshold(std::numeric_limits<double>::quiet_NaN()));
}

/// A double scale: 0.82*300/4 + 0.5 is just below 62, so the mean is 61 and a 62 pixel is above it
TEST_F(ThresholdBlockMeanTest, scale_in_double) {
    Gray<U8> input(2, 2);
    input.at(0, 0) = input.at(1, 0) = 62;
    input.at(0, 1) = input.at(1, 1) = 88;
    ThresholdBlockMean<U8> alg(ConfigLength::fixed(2), true, 0.82, true);
    Gray<U8> output(2, 2);
    alg.process(input, output);
    for (uint32_t y = 0; y < 2; y++)
        for (uint32_t x = 0; x < 2; x++)
            EXPECT_EQ(0, output.at(x, y)) << x << " " << y;
}

/// Like Java's (byte), a negative scaled mean keeps its low 8 bits: -49 is 207
TEST_F(ThresholdBlockMeanTest, negative_scale) {
    Gray<U8> input(4, 4);
    ImageMiscOps::fill(input, (U8)100);
    ThresholdBlockMean<U8> alg(ConfigLength::fixed(2), true, -0.5, true);
    Gray<U8> output(4, 4);
    alg.process(input, output);
    for (uint32_t y = 0; y < 4; y++)
        for (uint32_t x = 0; x < 4; x++)
            EXPECT_EQ(1, output.at(x, y)) << x << " " << y;
}

/// A double scale: 0.82*(45 + 255)/2 is just below 123, so the threshold is 122 and a 123 pixel is above it
TEST_F(ThresholdBlockMinMaxTest, scale_in_double) {
    Gray<U8> input(6, 6);
    const U8 values[] = {45, 123, 255};
    for (uint32_t y = 0; y < 6; y++)
        for (uint32_t x = 0; x < 6; x++)
            input.at(x, y) = values[(x + y)%3];
    ThresholdBlockMinMax<U8> alg(-1, ConfigLength::fixed(6), true, 0.82, true);
    Gray<U8> output(6, 6);
    alg.process(input, output);
    for (uint32_t y = 0; y < 6; y++)
        for (uint32_t x = 0; x < 6; x++)
            EXPECT_EQ(input.at(x, y) == 45, output.at(x, y) == 1) << x << " " << y;
}

/// A negative scale gives a negative threshold, which no pixel is at or below
TEST_F(ThresholdBlockMinMaxTest, negative_scale) {
    Gray<U8> input(6, 6);
    for (uint32_t y = 0; y < 6; y++)
        for (uint32_t x = 0; x < 6; x++)
            input.at(x, y) = (U8)(x*40 + y);
    for (bool down : {true, false}) {
        ThresholdBlockMinMax<U8> alg(-1, ConfigLength::fixed(3), true, -0.5, down);
        Gray<U8> output(6, 6);
        alg.process(input, output);
        for (uint32_t y = 0; y < 6; y++)
            for (uint32_t x = 0; x < 6; x++)
                EXPECT_EQ(down ? 0 : 1, output.at(x, y)) << down << " " << x << " " << y;
    }
}

/// Every block filter rejects a block width of zero instead of dividing by it
TEST_F(ThresholdBlockMeanTest, zero_block_width) {
    Gray<U8> input(10, 12);
    Gray<U8> output(10, 12);
    ThresholdBlockMean<U8> alg(ConfigLength::fixed(0), true, 1.0, true);
    EXPECT_THROW(alg.process(input, output), std::invalid_argument);
}
