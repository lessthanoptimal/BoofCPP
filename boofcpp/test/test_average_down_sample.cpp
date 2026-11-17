#include "gtest/gtest.h"
#include "average_down_sample.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>

using namespace std;
using namespace boofcv;

/// Computes the mean of each region in double precision. Integers round half away from zero
template<class E>
void naive_down( const Gray<E>& input, uint32_t width, Gray<E>& output ) {
    output.reshape(AverageDownSampleOps::down_sample_size(input.width, width),
                   AverageDownSampleOps::down_sample_size(input.height, width));

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            double total = 0;
            uint32_t count = 0;
            for (uint32_t yy = y*width; yy < std::min(input.height, (y + 1)*width); yy++) {
                for (uint32_t xx = x*width; xx < std::min(input.width, (x + 1)*width); xx++) {
                    total += input.at(xx, yy);
                    count++;
                }
            }
            double mean = total/count;
            output.at(x, y) = std::is_integral<E>::value ? (E)std::round(mean) : (E)mean;
        }
    }
}

/// Compares against naive_down for several image sizes and region widths, including regions larger than the image
template<class E>
void compare_to_naive( E min_value, E max_value ) {
    std::mt19937 gen(0xBEEF);

    for (uint32_t height : {1, 7, 15, 16}) {
        for (uint32_t width : {1, 6, 20, 21}) {
            Gray<E> input(width, height);
            ImageMiscOps::fill_uniform(input, min_value, max_value, gen);

            for (uint32_t region = 1; region <= 6; region++) {
                Gray<E> found(1, 1), expected(1, 1);
                AverageDownSampleOps::down(input, region, found);
                naive_down(input, region, expected);
                check_equals(expected, found);
            }
        }
    }
}

TEST(AverageDownSampleOps, down_compare_to_naive_U8) { compare_to_naive<U8>(0, 255); }
TEST(AverageDownSampleOps, down_compare_to_naive_S8) { compare_to_naive<S8>(-128, 127); }
TEST(AverageDownSampleOps, down_compare_to_naive_U16) { compare_to_naive<U16>(0, 65535); }
TEST(AverageDownSampleOps, down_compare_to_naive_S16) { compare_to_naive<S16>(-32768, 32767); }
TEST(AverageDownSampleOps, down_compare_to_naive_S32) { compare_to_naive<S32>(-1000000, 1000000); }
TEST(AverageDownSampleOps, down_compare_to_naive_F32) { compare_to_naive<F32>(-100, 100); }
TEST(AverageDownSampleOps, down_compare_to_naive_F64) { compare_to_naive<F64>(-100, 100); }

/// Every pixel is the largest value, the largest sum MeanDivisor must divide exactly
template<class E>
void max_value() {
    Gray<E> input(23, 17);
    ImageMiscOps::fill(input, std::numeric_limits<E>::max());

    for (uint32_t region = 1; region <= 6; region++) {
        Gray<E> found(1, 1);
        AverageDownSampleOps::down(input, region, found);
        for (uint32_t y = 0; y < found.height; y++)
            for (uint32_t x = 0; x < found.width; x++)
                ASSERT_EQ(std::numeric_limits<E>::max(), found.at(x, y));
    }
}

TEST(AverageDownSampleOps, down_max_value_U8) { max_value<U8>(); }
TEST(AverageDownSampleOps, down_max_value_U16) { max_value<U16>(); }

/// Negative halves round away from zero
TEST(AverageDownSampleOps, down_signed_rounding) {
    Gray<S16> input(4, 2);
    input.at(0, 0) = -1; input.at(1, 0) = -1;   // -2/4 = -0.5 -> -1
    input.at(2, 0) = 1; input.at(3, 0) = 1;     //  2/4 =  0.5 ->  1

    Gray<S16> output(1, 1);
    AverageDownSampleOps::down(input, 2, output);
    EXPECT_EQ(-1, output.at(0, 0));
    EXPECT_EQ(1, output.at(1, 0));

    AverageDownSampleOps::down(input, 4, output);   // 0/8
    EXPECT_EQ(0, output.at(0, 0));
}

/// Input and output are sub-images
TEST(AverageDownSampleOps, down_subimage) {
    std::mt19937 gen(0xBEEF);
    Gray<U8> input(21, 15);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
    Gray<U8> sub_input = create_subimage(input);

    for (uint32_t region : {2, 3}) {
        Gray<U8> expected(1, 1);
        naive_down(input, region, expected);
        Gray<U8> found = create_subimage(expected);
        ImageMiscOps::fill(found, (U8)0);

        AverageDownSampleOps::down(sub_input, region, found);
        check_equals(expected, found);
    }
}

TEST(AverageDownSampleOps, down_sample_size) {
    EXPECT_EQ(5u, AverageDownSampleOps::down_sample_size(10, 2));
    EXPECT_EQ(6u, AverageDownSampleOps::down_sample_size(11, 2));
    EXPECT_EQ(1u, AverageDownSampleOps::down_sample_size(3, 5));
}

TEST(AverageDownSampleOps, down_zero_width) {
    Gray<U8> input(4, 4), output(1, 1);
    EXPECT_THROW(AverageDownSampleOps::down(input, 0, output), std::invalid_argument);
}
