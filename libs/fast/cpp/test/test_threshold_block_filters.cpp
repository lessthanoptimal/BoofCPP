#include "gtest/gtest.h"
#include "testing_utils.h"

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "boofcv/threshold_block_filters.h"
#include "variant_fast.h"
#include "shared/threshold_checks.h"

using namespace boofcv;

class ThresholdBlockMeanTest : public boofcv::BoofCPPTest {};
class ThresholdBlockOtsuTest : public boofcv::BoofCPPTest {};
class ThresholdBlockMinMaxTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMeanTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_mean<U8>()); }
TEST_F(ThresholdBlockMeanTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_mean<F32>()); }
TEST_F(ThresholdBlockOtsuTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_otsu<U8>()); }
TEST_F(ThresholdBlockOtsuTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_otsu<F32>()); }
TEST_F(ThresholdBlockMinMaxTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_min_max<U8>()); }
TEST_F(ThresholdBlockMinMaxTest, standard_tests_F32) {
    threshold_checks::block_all(threshold_checks::block_min_max<F32>());
}

namespace {
    enum class Content { UNIFORM, FLAT, SMOOTH, HARD };

    /// Test images. HARD has values which are hard for floating point: large and tiny magnitudes, negative values and
    /// signed zeros for F32 (when 'limited' is false), and long runs of equal values
    template<class E>
    Gray<E> create_image( uint32_t width, uint32_t height, Content content, bool limited, std::mt19937& gen ) {
        Gray<E> image(width, height);
        std::uniform_real_distribution<double> uniform(0, 255);
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                double v = 0;
                switch (content) {
                    case Content::UNIFORM: v = uniform(gen); break;
                    case Content::FLAT: v = 128; break;
                    case Content::SMOOTH: v = 100 + 60*std::sin(x*0.05)*std::cos(y*0.07) + uniform(gen)/50; break;
                    case Content::HARD: {
                        const uint32_t kind = (x/7 + y/5)%5;
                        if (kind == 0) v = 200;
                        else if (kind == 1) v = uniform(gen);
                        else if (std::is_integral_v<E> || limited) v = kind*50;
                        else if (kind == 2) v = 1e6 + uniform(gen);
                        else if (kind == 3) v = -uniform(gen)*1e-30;
                        else v = (x + y)%2 == 0 ? 0.0 : -0.0;
                        break;
                    }
                }
                image.at(x, y) = std::is_integral_v<E> ? (E)std::lround(v) : (E)v;
            }
        }
        return image;
    }

    const std::vector<std::pair<uint32_t, uint32_t>> SHAPES = {{16, 16}, {37, 29}, {100, 120}, {160, 47}, {401, 297}};
    const Content CONTENTS[] = {Content::UNIFORM, Content::FLAT, Content::SMOOTH, Content::HARD};

    /// Fast's filter gives the same image as strict's, for the whole image and a sub-image
    template<class E>
    void check_same( InputToBinary<Gray<E>>& strict, InputToBinary<Gray<E>>& fast, const Gray<E>& input ) {
        Gray<U8> expected(input.width, input.height), found(input.width, input.height);
        strict.process(input, expected);
        fast.process(input, found);
        check_equals(expected, found);

        Gray<E> sub_input = create_subimage(input);
        Gray<U8> sub_found = create_subimage(found);
        fast.process(sub_input, sub_found);
        check_equals(expected, sub_found);
        sub_input.subimage = false;
        sub_found.subimage = false;
    }

    /// Calls check(input, width) for every test image and block width that fits it
    template<class E, class Check>
    void for_each_input( bool limited, Check check ) {
        std::mt19937 gen(0xBEEF);
        for (auto shape : SHAPES) {
            for (Content content : CONTENTS) {
                Gray<E> input = create_image<E>(shape.first, shape.second, content, limited, gen);
                for (uint32_t width : {2u, 5u, 14u, 40u}) {
                    if (width > std::min(shape.first, shape.second))
                        continue;
                    check(input, ConfigLength::fixed(width));
                }
            }
        }
    }

    template<class E>
    void block_mean_same_as_strict() {
        for_each_input<E>(false, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (bool down : {true, false})
                    for (double scale : {0.95, 1.0, 1.3}) {
                        boofcv::ThresholdBlockMean<E> strict(width, local, scale, down);
                        boofcv::fast::ThresholdBlockMean<E> fast(width, local, scale, down);
                        check_same<E>(strict, fast, input);
                    }
        });
    }

    /// NaN pixels, including the first pixel of some blocks, which strict never replaces
    template<class Strict, class Fast>
    void nan_same_as_strict( Strict& strict, Fast& fast ) {
        std::mt19937 gen(0xBEEF);
        Gray<F32> input = create_image<F32>(100, 120, Content::UNIFORM, true, gen);
        for (uint32_t y = 0; y < input.height; y += 3)
            for (uint32_t x = 0; x < input.width; x += 7)
                input.at(x, y) = std::numeric_limits<F32>::quiet_NaN();
        check_same<F32>(strict, fast, input);
    }
}

TEST_F(ThresholdBlockMeanTest, same_as_strict_U8) { block_mean_same_as_strict<U8>(); }
TEST_F(ThresholdBlockMeanTest, same_as_strict_F32) { block_mean_same_as_strict<F32>(); }

TEST_F(ThresholdBlockMeanTest, nan_same_as_strict) {
    for (bool local : {true, false}) {
        boofcv::ThresholdBlockMean<F32> strict(ConfigLength::fixed(10), local, 1.0, true);
        boofcv::fast::ThresholdBlockMean<F32> fast(ConfigLength::fixed(10), local, 1.0, true);
        nan_same_as_strict(strict, fast);
    }
}

/// Every block filter rejects a block width of zero instead of dividing by it
TEST_F(ThresholdBlockMeanTest, zero_block_width) {
    Gray<U8> input(10, 12);
    Gray<U8> output(10, 12);
    boofcv::fast::ThresholdBlockMean<U8> alg(ConfigLength::fixed(0), true, 1.0, true);
    EXPECT_THROW(alg.process(input, output), std::invalid_argument);
}
