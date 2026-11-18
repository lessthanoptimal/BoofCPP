#include "gtest/gtest.h"
#include "boofcv/average_down_sample.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <string>
#include <vector>

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

/// BoofCV's ImplAverageDownSampleN and ImplAverageDownSample2, written plainly: each region's pixels are added row by
/// row in the image's type, then RegionMean. Floats depend on this order
template<class E>
void boofcv_down( const Gray<E>& input, uint32_t width, Gray<E>& output ) {
    output.reshape(AverageDownSampleOps::down_sample_size(input.width, width),
                   AverageDownSampleOps::down_sample_size(input.height, width));
    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            const uint32_t y1 = std::min(input.height, (y + 1)*width), x1 = std::min(input.width, (x + 1)*width);
            typename TypeInfo<E>::sum_type total = 0;
            for (uint32_t yy = y*width; yy < y1; yy++)
                for (uint32_t xx = x*width; xx < x1; xx++)
                    total += input.at(xx, yy);
            output.at(x, y) = RegionMean<E>((y1 - y*width)*(x1 - x*width))(total);
        }
    }
}

/// BoofCV's naivePixelHorizontal() and naivePixelVertical(): output pixel 'index' of a region which might extend
/// outside the input. 'pixel(i)' is the i-th input pixel along the row or column
template<class F, class Pixel>
F boofcv_border_mean( Pixel pixel, uint32_t length, float offset, float scale, uint32_t index ) {
    float x0 = index*scale + offset;
    const float x1 = x0 + scale;
    int32_t i0 = (int32_t)std::floor((double)x0);
    const int32_t i1 = (int32_t)std::min((double)length, std::ceil((double)x1));
    if (i0 < 0) {
        i0 = 0;
        x0 = 0.0f;
    }
    float area = 0.0f;
    F sum = 0;
    for (int32_t i = i0; i < i1; i++) {
        const float intersection = std::min(i + 1.0f, x1) - x0;
        area += intersection;
        sum += intersection*pixel(i);
        x0 = i + 1.0f;
    }
    return sum/area;
}

/// BoofCV's AverageDownSampleOps.horizontal() and vertical() one output pixel at a time. 'along' is the input's length
/// along the axis, 'count' the output's, and 'pixel(i)' the i-th input pixel. 'vertical' selects vertical()'s weight
/// for the last pixel
template<class F, class Pixel>
F boofcv_general_pixel( Pixel pixel, uint32_t along, uint32_t count, bool centered, bool vertical, uint32_t index ) {
    const float scale = along/(float)count;
    const float offset = centered ? -(scale - 1.0f)/2.0f : 0.0f;
    const uint32_t border1 = count*scale - offset > along ? count - 1 : count;
    if ((centered && index == 0) || index >= border1)
        return boofcv_border_mean<F>(pixel, along, offset, scale, index);

    const float x0 = index*scale + offset;
    const float x1 = (index + 1)*scale + offset;
    const int32_t i0 = (int32_t)x0;
    const int32_t i1 = (int32_t)x1;
    F sum = (vertical ? 1 - (x0 - i0) : i0 + 1 - x0)*pixel(i0);
    for (int32_t i = i0 + 1; i < i1; i++)
        sum += pixel(i);
    if (i1 < x1)
        sum += (vertical ? std::fmod(x1, 1.0f) : x1 - i1)*pixel(i1);
    return sum/scale;
}

/// Random pixels. Floats also get values which are hard for floating point: large magnitudes next to small ones,
/// which cancel, tiny magnitudes and signed zeros. Integers use the type's whole range, so S32 sums wrap
template<class E>
Gray<E> create_hard_image( uint32_t width, uint32_t height, std::mt19937& gen ) {
    Gray<E> image(width, height);
    std::uniform_real_distribution<double> uniform(0, 1);
    const double lo = std::is_floating_point_v<E> ? -100.0 : (double)std::numeric_limits<E>::lowest();
    const double hi = std::is_floating_point_v<E> ? 100.0 : (double)std::numeric_limits<E>::max();
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            double v = lo + (hi - lo)*uniform(gen);
            if constexpr (std::is_floating_point_v<E>) {
                switch ((x/3 + y/2)%6) {
                    case 0: v = (x%2 == 0 ? 1e7 : -1e7) + v; break;
                    case 1: v *= 1e-30; break;
                    case 2: v = (x + y)%2 == 0 ? 0.0 : -0.0; break;
                    default: break;
                }
            }
            image.at(x, y) = std::is_integral_v<E> ? (E)std::llround(std::clamp(v, lo, hi)) : (E)v;
        }
    }
    return image;
}

/// Sizes which aren't a multiple of the vector width or the region width, smaller and larger than the regions
const std::vector<std::pair<uint32_t, uint32_t>> HARD_SHAPES = {{1, 1}, {7, 3}, {16, 16}, {37, 29}, {101, 70},
                                                                {160, 47}};

/// Bit identical to BoofCV's order of operations for every region width, for the whole image and a sub-image
template<class E>
void down_same_as_boofcv() {
    std::mt19937 gen(0xBEEF);
    for (auto shape : HARD_SHAPES) {
        Gray<E> input = create_hard_image<E>(shape.first, shape.second, gen);
        Gray<E> sub_input = create_subimage(input);
        for (uint32_t width : {1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 70}) {
            Gray<E> expected(1, 1), found(1, 1);
            boofcv_down(input, width, expected);
            AverageDownSampleOps::down(input, width, found);
            check_identical(expected, found, "width " + std::to_string(width));

            Gray<E> sub_found = create_subimage(expected);
            AverageDownSampleOps::down(sub_input, width, sub_found);
            check_identical(expected, sub_found, "sub-image width " + std::to_string(width));
            sub_found.subimage = false;
        }
        sub_input.subimage = false;
    }
}

TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_U8) { down_same_as_boofcv<U8>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_S8) { down_same_as_boofcv<S8>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_U16) { down_same_as_boofcv<U16>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_S16) { down_same_as_boofcv<S16>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_S32) { down_same_as_boofcv<S32>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_F32) { down_same_as_boofcv<F32>(); }
TEST_F(AverageDownSampleOpsTest, down_same_as_boofcv_F64) { down_same_as_boofcv<F64>(); }

/// Output sizes with integer and non-integer scales, the input's size and one pixel
std::vector<uint32_t> general_lengths( uint32_t length ) {
    return {length, std::max(1u, length/2), length/3 + 1, std::max(1u, length*2/5), 2, 1};
}

/// The horizontal and vertical passes and the general down sample are bit identical to BoofCV's order of operations
template<class E>
void general_same_as_boofcv() {
    typedef AverageDownSampleOps::middle_type<E> F;
    std::mt19937 gen(0xBEEF);
    for (auto shape : HARD_SHAPES) {
        Gray<E> input = create_hard_image<E>(shape.first, shape.second, gen);
        Gray<E> sub_input = create_subimage(input);
        for (uint32_t width : general_lengths(input.width)) {
            for (uint32_t height : general_lengths(input.height)) {
                for (bool centered : {false, true}) {
                    // The output can't be larger than the input. BoofCV writes past the end of a row for a centered
                    // output one pixel wide
                    if (width > input.width || height > input.height || (centered && width == 1 && input.width > 1))
                        continue;
                    Gray<F> expected_middle(width, input.height);
                    for (uint32_t y = 0; y < input.height; y++) {
                        for (uint32_t x = 0; x < width; x++) {
                            auto pixel = [&]( int32_t i ) { return input.at(i, y); };
                            expected_middle.at(x, y) = boofcv_general_pixel<F>(pixel, input.width, width, centered,
                                                                               false, x);
                        }
                    }
                    Gray<E> expected(width, height);
                    for (uint32_t y = 0; y < height; y++) {
                        for (uint32_t x = 0; x < width; x++) {
                            auto pixel = [&]( int32_t i ) { return expected_middle.at(x, i); };
                            F mean = boofcv_general_pixel<F>(pixel, input.height, height, centered, true, y);
                            expected.at(x, y) = std::is_same_v<E, F> ? (E)mean : (E)(int32_t)(mean + 0.5f);
                        }
                    }

                    Gray<E> found(width, height);
                    Gray<F> found_middle(1, 1);
                    AverageDownSampleOps::down(sub_input, centered, found, found_middle);
                    check_identical(expected_middle, found_middle, "middle");
                    check_identical(expected, found, "output");
                }
            }
        }
        sub_input.subimage = false;
    }
}

TEST_F(AverageDownSampleOpsTest, general_same_as_boofcv_U8) { general_same_as_boofcv<U8>(); }
TEST_F(AverageDownSampleOpsTest, general_same_as_boofcv_U16) { general_same_as_boofcv<U16>(); }
TEST_F(AverageDownSampleOpsTest, general_same_as_boofcv_F32) { general_same_as_boofcv<F32>(); }
TEST_F(AverageDownSampleOpsTest, general_same_as_boofcv_F64) { general_same_as_boofcv<F64>(); }
