#include "gtest/gtest.h"
#include "boofcv/pixel_math.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <limits>
#include <random>

using namespace std;
using namespace boofcv;

namespace {
    Gray<F32> random_F32( uint32_t width, uint32_t height, F32 min_value, F32 max_value ) {
        Gray<F32> image(width, height);
        std::mt19937 gen(0xBEEF + width);
        ImageMiscOps::fill_uniform(image, min_value, max_value, gen);
        return image;
    }
}

TEST(PixelMath, pow2) {
    Gray<F32> input = random_F32(12, 9, -5.0f, 5.0f);
    for (bool sub : {false, true}) {
        Gray<F32> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, 12, 9);
        Gray<F32> output(1, 1);
        PixelMath::pow2(in, output);
        for (uint32_t y = 0; y < 9; y++) {
            for (uint32_t x = 0; x < 12; x++)
                ASSERT_EQ(input.at(x, y)*input.at(x, y), output.at(x, y));
        }
        if (sub)
            in.subimage = false;
    }
}

TEST(PixelMath, stdev) {
    Gray<F32> mean = random_F32(12, 9, -5.0f, 5.0f);
    Gray<F32> pow2(12, 9);
    for (uint32_t y = 0; y < 9; y++) {
        for (uint32_t x = 0; x < 12; x++) {
            F32 m = mean.at(x, y);
            // variance from 0 to 3, except one that's negative due to "numerical error"
            pow2.at(x, y) = m*m + (F32)((x + y)%4);
        }
    }
    pow2.at(3, 2) = mean.at(3, 2)*mean.at(3, 2) - 0.5f;

    Gray<F32> found(1, 1);
    PixelMath::stdev(mean, pow2, found);
    for (uint32_t y = 0; y < 9; y++) {
        for (uint32_t x = 0; x < 12; x++) {
            F32 m = mean.at(x, y);
            F32 expected = (F32)std::sqrt((double)std::max(0.0f, pow2.at(x, y) - m*m));
            ASSERT_EQ(expected, found.at(x, y)) << x << " " << y;
        }
    }
    EXPECT_EQ(0.0f, found.at(3, 2));

    // In place, which is how NCC uses it
    PixelMath::stdev(mean, pow2, pow2);
    check_equals(found, pow2, 0.0f);

    // NaN stays NaN, same as Java's Math.max()
    pow2.at(0, 0) = std::numeric_limits<F32>::quiet_NaN();
    PixelMath::stdev(mean, pow2, found);
    EXPECT_TRUE(std::isnan(found.at(0, 0)));
}

TEST(PixelMath, plus_minus_multiply) {
    Gray<U8> input(12, 9);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
    for (bool sub : {false, true}) {
        Gray<U8> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, 12, 9);
        Gray<F32> plus(1, 1), minus(1, 1), multiply(1, 1);
        PixelMath::plus(in, 1.5f, plus);
        PixelMath::minus(in, 1.5f, minus);
        PixelMath::multiply(in, 0.1f, multiply);
        for (uint32_t y = 0; y < 9; y++) {
            for (uint32_t x = 0; x < 12; x++) {
                F32 v = (F32)input.at(x, y);
                ASSERT_EQ(v + 1.5f, plus.at(x, y));
                ASSERT_EQ(v - 1.5f, minus.at(x, y));
                ASSERT_EQ(v*0.1f, multiply.at(x, y));
            }
        }
        if (sub)
            in.subimage = false;
    }

    // F32 in place
    Gray<F32> image = random_F32(7, 5, -2.0f, 2.0f);
    Gray<F32> copy(7, 5);
    for (uint32_t i = 0; i < image.data_length; i++)
        copy.data[i] = image.data[i];
    PixelMath::multiply(image, 3.0f, image);
    PixelMath::minus(image, 0.25f, image);
    for (uint32_t i = 0; i < image.data_length; i++)
        ASSERT_EQ(copy.data[i]*3.0f - 0.25f, image.data[i]);
}
