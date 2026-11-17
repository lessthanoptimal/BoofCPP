#include "gtest/gtest.h"
#include "boofcv/average_down_sample.h"
#include "boofcv/image_misc_ops.h"
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

class AverageDownSampleOpsTest : public boofcv::BoofCPPTest {};

TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_U8) { compare_to_naive<U8>(0, 255); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_S8) { compare_to_naive<S8>(-128, 127); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_U16) { compare_to_naive<U16>(0, 65535); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_S16) { compare_to_naive<S16>(-32768, 32767); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_S32) { compare_to_naive<S32>(-1000000, 1000000); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_F32) { compare_to_naive<F32>(-100, 100); }
TEST_F(AverageDownSampleOpsTest, down_compare_to_naive_F64) { compare_to_naive<F64>(-100, 100); }

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

TEST_F(AverageDownSampleOpsTest, down_max_value_U8) { max_value<U8>(); }
TEST_F(AverageDownSampleOpsTest, down_max_value_U16) { max_value<U16>(); }

/// Negative halves round away from zero
TEST_F(AverageDownSampleOpsTest, down_signed_rounding) {
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
TEST_F(AverageDownSampleOpsTest, down_subimage) {
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

TEST_F(AverageDownSampleOpsTest, down_sample_size) {
    EXPECT_EQ(5u, AverageDownSampleOps::down_sample_size(10, 2));
    EXPECT_EQ(6u, AverageDownSampleOps::down_sample_size(11, 2));
    EXPECT_EQ(1u, AverageDownSampleOps::down_sample_size(3, 5));
}

TEST_F(AverageDownSampleOpsTest, down_zero_width) {
    Gray<U8> input(4, 4), output(1, 1);
    EXPECT_THROW(AverageDownSampleOps::down(input, 0, output), std::invalid_argument);
}

/// Weight of each input pixel along one axis for output pixel 'index', in double precision. A region has length
/// input/output and is shifted by -(scale-1)/2 if centered. The weight is the overlap of a pixel with the region
/// divided by the length of the region inside the input
std::vector<double> naive_weights( uint32_t input_length, uint32_t output_length, bool centered, uint32_t index ) {
    const double scale = (double)input_length/output_length;
    const double offset = centered ? -(scale - 1.0)/2.0 : 0.0;
    const double r0 = std::max(0.0, index*scale + offset);
    const double r1 = std::min((double)input_length, (index + 1)*scale + offset);

    std::vector<double> weights(input_length, 0.0);
    for (uint32_t i = 0; i < input_length; i++) {
        weights[i] = std::max(0.0, std::min(i + 1.0, r1) - std::max((double)i, r0))/(r1 - r0);
    }
    return weights;
}

/// Mean of the region for each output pixel, in double precision. Row-major with output_width columns
template<class E>
std::vector<double> naive_general( const Gray<E>& input, bool centered, uint32_t output_width, uint32_t output_height ) {
    std::vector<double> output(output_width*output_height, 0.0);
    for (uint32_t y = 0; y < output_height; y++) {
        std::vector<double> wy = naive_weights(input.height, output_height, centered, y);
        for (uint32_t x = 0; x < output_width; x++) {
            std::vector<double> wx = naive_weights(input.width, output_width, centered, x);
            double total = 0;
            for (uint32_t j = 0; j < input.height; j++)
                for (uint32_t i = 0; i < input.width; i++)
                    total += wx[i]*wy[j]*input.at(i, j);
            output[y*output_width + x] = total;
        }
    }
    return output;
}

/// Integers round to the nearest value, which can differ by 1 from BoofCV's float math at a half
template<class E>
void check_general( const std::vector<double>& expected, const Gray<E>& found ) {
    for (uint32_t y = 0; y < found.height; y++) {
        for (uint32_t x = 0; x < found.width; x++) {
            double value = expected[y*found.width + x];
            if (std::is_integral<E>::value)
                ASSERT_NEAR(std::floor(value + 0.5), found.at(x, y), 1.0) << x << " " << y;
            else
                ASSERT_NEAR(value, found.at(x, y), 1e-3) << x << " " << y;
        }
    }
}

/// Output sizes with integer and non-integer scales, the same size as the input, and one pixel
const std::vector<std::pair<uint32_t, uint32_t>> GENERAL_SHAPES = {{21, 16}, {10, 8}, {7, 5}, {6, 11}, {3, 2}, {2, 1}};

template<class E>
void general_compare_to_naive() {
    std::mt19937 gen(0xBEEF);
    Gray<E> input(21, 16);
    ImageMiscOps::fill_uniform(input, (E)0, (E)255, gen);
    Gray<E> sub_input = create_subimage(input);

    for (bool centered : {false, true}) {
        for (auto shape : GENERAL_SHAPES) {
            Gray<E> found(shape.first, shape.second);
            Gray<AverageDownSampleOps::middle_type<E>> middle(1, 1);
            AverageDownSampleOps::down(sub_input, centered, found, middle);
            check_general(naive_general(input, centered, shape.first, shape.second), found);
        }
    }
}

TEST_F(AverageDownSampleOpsTest, general_compare_to_naive_U8) { general_compare_to_naive<U8>(); }
TEST_F(AverageDownSampleOpsTest, general_compare_to_naive_U16) { general_compare_to_naive<U16>(); }
TEST_F(AverageDownSampleOpsTest, general_compare_to_naive_F32) { general_compare_to_naive<F32>(); }
TEST_F(AverageDownSampleOpsTest, general_compare_to_naive_F64) { general_compare_to_naive<F64>(); }

/// The horizontal pass alone is the general down sample with the input's height
TEST_F(AverageDownSampleOpsTest, horizontal) {
    std::mt19937 gen(0xBEEF);
    Gray<U8> input(21, 16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);

    for (bool centered : {false, true}) {
        for (uint32_t width : {21, 10, 7, 2}) {
            Gray<F32> found(width, input.height);
            AverageDownSampleOps::horizontal(input, centered, found);
            check_general(naive_general(input, centered, width, input.height), found);
        }
    }
}

/// The vertical pass alone is the general down sample with the input's width. Integer outputs round
TEST_F(AverageDownSampleOpsTest, vertical) {
    std::mt19937 gen(0xBEEF);
    Gray<F32> input(21, 16);
    ImageMiscOps::fill_uniform(input, 0.0f, 255.0f, gen);

    for (bool centered : {false, true}) {
        for (uint32_t height : {16, 8, 5, 1}) {
            Gray<U8> found_u8(input.width, height);
            AverageDownSampleOps::vertical(input, centered, found_u8);
            check_general(naive_general(input, centered, input.width, height), found_u8);

            Gray<F32> found_f32(input.width, height);
            AverageDownSampleOps::vertical(input, centered, found_f32);
            check_general(naive_general(input, centered, input.width, height), found_f32);
        }
    }
}

/// The output can't be larger than the input, and a centered output can't be one pixel wide
TEST_F(AverageDownSampleOpsTest, general_bad_shapes) {
    Gray<U8> input(10, 8);
    Gray<F32> middle(5, 8);
    Gray<U8> output(5, 4);

    Gray<F32> wide(11, 8), short_middle(5, 7), narrow(1, 8);
    EXPECT_THROW(AverageDownSampleOps::horizontal(input, false, wide), std::invalid_argument);
    EXPECT_THROW(AverageDownSampleOps::horizontal(input, false, short_middle), std::invalid_argument);
    EXPECT_THROW(AverageDownSampleOps::horizontal(input, true, narrow), std::invalid_argument);
    AverageDownSampleOps::horizontal(input, false, narrow);

    Gray<U8> tall(5, 9), thin(4, 4);
    EXPECT_THROW(AverageDownSampleOps::vertical(middle, false, tall), std::invalid_argument);
    EXPECT_THROW(AverageDownSampleOps::vertical(middle, false, thin), std::invalid_argument);
    AverageDownSampleOps::vertical(middle, true, output);
}
