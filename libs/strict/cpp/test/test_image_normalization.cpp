#include "gtest/gtest.h"
#include "boofcv/image_normalization.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>

using namespace std;
using namespace boofcv;

namespace {
    /// The output is (input + offset)/divisor and has a mean of zero. The maximum absolute value is one for signed
    /// images. Unsigned images are scaled as if the pixels go from 0 to the largest value, so it's at most one
    template<class E>
    void check_zero_mean_max_one( const Gray<E>& input ) {
        Gray<F32> output(1, 1);
        NormalizeParameters parameters;
        ImageNormalization::zero_mean_max_one(input, output, parameters);
        ASSERT_EQ(input.width, output.width);
        ASSERT_EQ(input.height, output.height);

        double mean = 0, max_abs = 0;
        for (uint32_t y = 0; y < input.height; y++) {
            for (uint32_t x = 0; x < input.width; x++) {
                double expected = ((double)input.at(x, y) + parameters.offset)/parameters.divisor;
                ASSERT_NEAR(expected, output.at(x, y), 1e-5);
                mean += output.at(x, y);
                max_abs = std::max(max_abs, (double)std::abs(output.at(x, y)));
            }
        }
        EXPECT_NEAR(0.0, mean/(input.width*input.height), 1e-5);
        if (std::is_signed<E>::value)
            EXPECT_NEAR(1.0, max_abs, 1e-5);
        else
            EXPECT_LE(max_abs, 1.0 + 1e-5);

        // apply() with the same parameters gives the same image
        Gray<F32> applied(1, 1);
        ImageNormalization::apply(input, parameters, applied);
        check_equals(output, applied, 1e-5f);
    }
}

class ImageNormalizationTest : public boofcv::BoofCPPTest {};

TEST_F(ImageNormalizationTest, zero_mean_max_one_U8) {
    Gray<U8> input(20, 15);
    ImageMiscOps::fill_uniform(input, (U8)10, (U8)200, rng);
    check_zero_mean_max_one(input);
    check_zero_mean_max_one(create_subimage(input));

    // scaled by 1 - mean if the mean is less than 0.5 after dividing by the largest value, otherwise by the mean
    Gray<F32> output(1, 1);
    for (U8 v : {(U8)20, (U8)250}) {
        ImageMiscOps::fill(input, (U8)100);
        input.at(0, 0) = v;
        NormalizeParameters parameters;
        ImageNormalization::zero_mean_max_one(input, output, parameters);
        double scale = std::max(100, (int)v);
        double mean = (100.0*(input.data_length - 1) + v)/input.data_length/scale;
        double scale2 = mean < 0.5 ? 1.0 - mean : mean;
        // the mean is summed in float, so relative to the value
        EXPECT_NEAR(scale*scale2, parameters.divisor, 1e-5*scale*scale2) << (int)v;
        EXPECT_NEAR(-mean*scale, parameters.offset, 1e-5*mean*scale) << (int)v;
    }
}

TEST_F(ImageNormalizationTest, zero_mean_max_one_F32) {
    Gray<F32> input(20, 15);
    ImageMiscOps::fill_uniform(input, -3.0f, 7.0f, rng);
    check_zero_mean_max_one(input);
    check_zero_mean_max_one(create_subimage(input));
}

/// An image of zeros can't be scaled, so nothing is changed
TEST_F(ImageNormalizationTest, zero_mean_max_one_all_zero) {
    Gray<U8> input(6, 4);
    ImageMiscOps::fill(input, (U8)0);
    Gray<F32> output(6, 4);
    ImageMiscOps::fill(output, 2.0f);
    NormalizeParameters parameters{3, 4};
    ImageNormalization::zero_mean_max_one(input, output, parameters);
    EXPECT_EQ(0.0, parameters.offset);
    EXPECT_EQ(1.0, parameters.divisor);
    for (uint32_t i = 0; i < output.data_length; i++)
        ASSERT_EQ(2.0f, output.data[i]);
}

TEST_F(ImageNormalizationTest, max_abs) {
    Gray<F32> f32(5, 4);
    ImageMiscOps::fill(f32, 1.0f);
    f32.at(2, 3) = -6.5f;
    f32.at(4, 1) = 6.0f;
    EXPECT_EQ(6.5f, ImageNormalization::max_abs(f32));

    Gray<U8> u8(5, 4);
    ImageMiscOps::fill(u8, (U8)3);
    u8.at(1, 2) = 250;
    EXPECT_EQ(250, ImageNormalization::max_abs(u8));
}

/// Summed in a single float in row major order, like BoofCV
TEST_F(ImageNormalizationTest, mean_F32) {
    Gray<F32> image(7, 3);
    ImageMiscOps::fill_uniform(image, 0.0f, 1.0f, rng);
    F32 total = 0;
    for (uint32_t y = 0; y < 3; y++) {
        for (uint32_t x = 0; x < 7; x++)
            total += image.at(x, y);
    }
    EXPECT_EQ(total/21.0f, ImageNormalization::mean_F32(image));
    EXPECT_EQ(total/21.0f, ImageNormalization::mean_F32(create_subimage(image)));
}
