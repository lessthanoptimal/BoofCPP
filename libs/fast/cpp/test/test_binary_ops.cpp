#include "gtest/gtest.h"
#include "testing_utils.h"

#include <random>
#include <stdexcept>

#include "boofcv/binary_ops.h"
#include "boofcv/image_convert.h"
#include "variant_fast.h"
#include "shared/threshold_checks.h"

using namespace boofcv;

class GlobalOtsuBinaryFilterTest : public boofcv::BoofCPPTest {};
class GlobalFixedBinaryFilterTest : public boofcv::BoofCPPTest {};
class LocalMeanBinaryFilterTest : public boofcv::BoofCPPTest {};
class ThresholdOpsTest : public boofcv::BoofCPPTest {};

TEST_F(GlobalOtsuBinaryFilterTest, scale) { threshold_checks::global_otsu_scale(); }
TEST_F(GlobalOtsuBinaryFilterTest, min_value_offset) { threshold_checks::global_otsu_min_value_offset(); }
TEST_F(GlobalFixedBinaryFilterTest, double_threshold) { threshold_checks::global_fixed_double_threshold(); }
TEST_F(LocalMeanBinaryFilterTest, naive) { threshold_checks::local_mean_naive(); }
TEST_F(LocalMeanBinaryFilterTest, zero_radius_U8) { threshold_checks::local_mean_zero_radius<U8>(); }
TEST_F(LocalMeanBinaryFilterTest, zero_radius_F32) { threshold_checks::local_mean_zero_radius<F32>(); }

namespace {
    /// Random U8 image with values in [low, high], or a flat one if low == high
    Gray<U8> random_image( uint32_t width, uint32_t height, int low, int high, std::mt19937& gen ) {
        Gray<U8> image(width, height);
        ImageMiscOps::fill_uniform(image, (U8)low, (U8)high, gen);
        return image;
    }

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
}

/// Same threshold as strict, for histograms with any offset, and the same exceptions
TEST_F(ThresholdOpsTest, computeOtsu_same_as_strict) {
    std::mt19937 gen(0xBEEF);
    int inside = 0, outside = 0;
    for (auto shape : {std::pair<uint32_t, uint32_t>{1, 1}, {7, 3}, {37, 29}, {401, 297}}) {
        for (auto values : {std::pair<int, int>{0, 255}, {20, 200}, {90, 90}, {0, 3}}) {
            Gray<U8> input = random_image(shape.first, shape.second, values.first, values.second, gen);
            for (auto bounds : {std::pair<double, double>{0, 255}, {20, 200}, {-5.5, 300.7}, {90, 90}, {0, 1e6}}) {
                for (bool otsu2 : {true, false}) {
                    uint32_t expected = 0;
                    bool thrown = false;
                    try {
                        expected = boofcv::ThresholdOps::computeOtsu(input, bounds.first, bounds.second, otsu2);
                    } catch (std::out_of_range&) {
                        thrown = true;
                    }
                    if (thrown) {
                        EXPECT_THROW(boofcv::fast::ThresholdOps::computeOtsu(input, bounds.first, bounds.second, otsu2),
                                     std::out_of_range);
                        outside++;
                    } else {
                        ASSERT_EQ(expected,
                                  boofcv::fast::ThresholdOps::computeOtsu(input, bounds.first, bounds.second, otsu2));
                        inside++;
                    }
                }
            }
        }
    }
    // both cases were tested
    EXPECT_GT(inside, 0);
    EXPECT_GT(outside, 0);

    Gray<U8> input(5, 5);
    EXPECT_THROW(boofcv::fast::ThresholdOps::computeOtsu(input, 10.0, 5.0), std::invalid_argument);
    EXPECT_THROW(boofcv::fast::ThresholdOps::computeOtsu(input, -1e12, 5.0), std::invalid_argument);
}

TEST_F(GlobalOtsuBinaryFilterTest, same_as_strict) {
    std::mt19937 gen(0xBEEF);
    for (auto shape : {std::pair<uint32_t, uint32_t>{7, 3}, {37, 29}, {401, 297}}) {
        Gray<U8> input = random_image(shape.first, shape.second, 0, 255, gen);
        Gray<F32> inputF;
        convert(input, inputF);
        for (bool down : {true, false}) {
            for (double scale : {0.0, 0.95, 1.0, 1.3}) {
                boofcv::GlobalOtsuBinaryFilter<U8> strict(0, 255, down, scale);
                boofcv::fast::GlobalOtsuBinaryFilter<U8> fast(0, 255, down, scale);
                check_same<U8>(strict, fast, input);
                boofcv::GlobalOtsuBinaryFilter<F32> strictF(0, 255, down, scale);
                boofcv::fast::GlobalOtsuBinaryFilter<F32> fastF(0, 255, down, scale);
                check_same<F32>(strictF, fastF, inputF);
            }
        }
    }
}

/// Same as strict on random, flat and smooth images, where many pixels equal their mean, with regions from 2 pixels to
/// wider than the image
TEST_F(LocalMeanBinaryFilterTest, same_as_strict) {
    std::mt19937 gen(0xBEEF);
    for (auto shape : {std::pair<uint32_t, uint32_t>{2, 2}, {5, 3}, {37, 29}, {64, 64}, {401, 297}}) {
        for (int content = 0; content < 3; content++) {
            Gray<U8> input = random_image(shape.first, shape.second, 0, 255, gen);
            if (content == 1) {
                ImageMiscOps::fill(input, (U8)128);
            } else if (content == 2) {
                for (uint32_t y = 0; y < input.height; y++)
                    for (uint32_t x = 0; x < input.width; x++)
                        input.at(x, y) = (U8)((x*3 + y*2)%256);
            }
            Gray<F32> inputF;
            convert(input, inputF);
            for (double region : {2.0, 3.0, 4.0, 7.0, 41.0, 1000.0}) {
                for (bool down : {true, false}) {
                    for (float scale : {0.95f, 1.0f, 1.05f}) {
                        boofcv::LocalMeanBinaryFilter<U8> strict(ConfigLength::fixed(region), scale, down);
                        boofcv::fast::LocalMeanBinaryFilter<U8> fast(ConfigLength::fixed(region), scale, down);
                        check_same<U8>(strict, fast, input);
                    }
                }
            }
            boofcv::LocalMeanBinaryFilter<F32> strictF(ConfigLength::fixed(7), 0.95f, true);
            boofcv::fast::LocalMeanBinaryFilter<F32> fastF(ConfigLength::fixed(7), 0.95f, true);
            check_same<F32>(strictF, fastF, inputF);
        }
    }
}

/// A region relative to the image size, as BoofCV's default configurations have
TEST_F(LocalMeanBinaryFilterTest, relative_region) {
    std::mt19937 gen(0xBEEF);
    Gray<U8> input = random_image(160, 47, 0, 255, gen);
    boofcv::LocalMeanBinaryFilter<U8> strict(ConfigLength::relative(0.2, 3), 0.95f, true);
    boofcv::fast::LocalMeanBinaryFilter<U8> fast(ConfigLength::relative(0.2, 3), 0.95f, true);
    check_same<U8>(strict, fast, input);
}
