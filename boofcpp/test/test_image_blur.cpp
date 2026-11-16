#include "gtest/gtest.h"
#include "image_blur.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

#include <random>

#include "print_structures.h"

using namespace std;
using namespace boofcv;

template<class E>
class CompareToNormalized {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;

    std::mt19937 gen;

    uint32_t borderX0,borderX1;
    uint32_t borderY0,borderY1;

    CompareToNormalized() : gen(0xBEEF) {

    }

    void setImageSize( uint32_t width , uint32_t height ) {

        input.reshape(width,height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)50,gen);
        found.reshape(width,height);
        expected.reshape(width,height);
    }

    void setMeanRadius(uint32_t radius) {
        kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
    }

    void horizontal_mean_inner() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveImageMean::inner_horizontal(input,found,kernel.width/2);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void vertical_mean_inner() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveImageMean::inner_vertical(input,found,kernel.width/2);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void horizontal_mean() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveImageMean::horizontal(input,found,kernel.width/2);

        check_equals(expected,found);
    }

    void vertical_mean() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveImageMean::vertical(input,found,kernel.width/2);

        check_equals(expected,found);
    }

    void mean() {
        Gray<E> tmp(input.width,input.height);
        ConvolveNormalizedNaive::horizontal(kernel,input,tmp);
        ConvolveNormalizedNaive::vertical(kernel,tmp,expected);
        BlurImageOps::mean(input,found,kernel.width/2,tmp);

        check_equals(expected,found);
    }

    void gaussian( int32_t kernel_width ) {

        kernel = FactoryKernel::gaussian1D<signed_type>(-1,kernel_width);

        Gray<E> tmp(input.width,input.height);
        ConvolveNormalizedNaive::horizontal(kernel,input,tmp);
        ConvolveNormalizedNaive::vertical(kernel,tmp,expected);
        BlurImageOps::gaussian(input,found,-1,kernel_width,tmp);

        check_equals(expected,found);
    }

    void checkResults_inner() {
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1);
    }
};

namespace {
    /// MeanDivisor must give the same result as BoofCV's (total + length/2)/length for every sum in [n0, n1)
    template<class E>
    void check_mean_divisor( uint32_t length, int64_t n0, int64_t n1 ) {
        const MeanDivisor<E> divisor(length);
        for (int64_t n = n0; n < n1; n++) {
            auto total = (typename TypeInfo<E>::sum_type)n;
            auto expected = (E)((total + (typename TypeInfo<E>::sum_type)(length/2))/length);
            ASSERT_EQ(expected, divisor(total)) << "length " << length << " sum " << n;
            if (divisor.uses_multiply())
                ASSERT_EQ(expected, divisor.multiply(total)) << "length " << length << " sum " << n;
            if (divisor.uses_multiply32())
                ASSERT_EQ(expected, divisor.multiply32(total)) << "length " << length << " sum " << n;
        }
    }

    /// Every possible sum of 'length' pixels, or the smallest and largest million and every 997th in between
    template<class E>
    void check_mean_divisor( uint32_t length ) {
        const int64_t n_max = (int64_t)std::numeric_limits<E>::max()*length;
        if (n_max <= 4'000'000) {
            check_mean_divisor<E>(length, 0, n_max + 1);
            return;
        }
        check_mean_divisor<E>(length, 0, 1'000'000);
        check_mean_divisor<E>(length, n_max - 1'000'000, n_max + 1);
        for (int64_t n = 1'000'000; n < n_max - 1'000'000; n += 997)
            check_mean_divisor<E>(length, n, n + 1);
    }
}

TEST(MeanDivisor, U8) {
    for (uint32_t length = 1; length <= 101; length++)
        check_mean_divisor<U8>(length);
    // 8405024 is the largest length that uses a multiplication, 8405025 divides
    for (uint32_t length : {255u, 256u, 1001u, 65537u, 8405024u, 8405025u})
        check_mean_divisor<U8>(length);
}

TEST(MeanDivisor, U16) {
    for (uint32_t length = 1; length <= 61; length++)
        check_mean_divisor<U16>(length);
    // 32768 is the largest length that uses a multiplication, 32769 divides
    for (uint32_t length : {255u, 1001u, 32768u, 32769u, 65535u})
        check_mean_divisor<U16>(length);
}

/// The 32-bit multiplication is used when the products fit, e.g. U8 Gaussian kernels, but not for U16
TEST(MeanDivisor, multiply32) {
    ASSERT_TRUE(MeanDivisor<U8>(71).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U8>(11).uses_multiply32());
    ASSERT_FALSE(MeanDivisor<U8>(8405024).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U8>(8405024).uses_multiply());
    ASSERT_FALSE(MeanDivisor<U16>(71).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U16>(71).uses_multiply());
}

TEST(MeanDivisor, signed_and_float) {
    const MeanDivisor<S16> divisor_s16(7);
    for (int32_t n = -40000; n <= 40000; n++)
        ASSERT_EQ((S16)((n + 3)/7), divisor_s16(n));
    const MeanDivisor<F32> divisor_f32(7);
    const MeanDivisor<F64> divisor_f64(7);
    for (float n : {0.0f, -0.0f, 1.5f, 100.25f, -3.0f})
        ASSERT_EQ(n/7.0f, divisor_f32(n));
    for (double n : {0.0, 1.5, 100.25, -3.0})
        ASSERT_EQ(n/7.0, divisor_f64(n));
    ASSERT_TRUE(std::signbit(divisor_f32(-0.0f)));
}

TEST(ConvolveImageMean, inner_horizontal_U8) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(2);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_horizontal_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(2);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_U8) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean_inner();
        compare.setMeanRadius(2);
        compare.vertical_mean_inner();
        compare.setMeanRadius(5);
        compare.vertical_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean_inner();
        compare.setMeanRadius(2);
        compare.vertical_mean_inner();
        compare.setMeanRadius(5);
        compare.vertical_mean_inner();
    }
}

TEST(ConvolveImageMean, horizonal) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.horizontal_mean();
        compare.setMeanRadius(2);
        compare.horizontal_mean();
        compare.setMeanRadius(5);
        compare.horizontal_mean();
    }
}

TEST(ConvolveImageMean, vertical) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean();
        compare.setMeanRadius(2);
        compare.vertical_mean();
        compare.setMeanRadius(5);
        compare.vertical_mean();
    }
}

TEST(BlurImageOps, mean) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 3; i++ ) {
        uint32_t w = 3+i;
        uint32_t h = 4+i;
//        printf("mean i = %d  %dx%d\n",i,w,h);

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.mean();
        compare.setMeanRadius(2);
        compare.mean();
        compare.setMeanRadius(5);
        compare.mean();
    }
}

TEST(BlurImageOps, gaussian) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 3; i++ ) {
        uint32_t w = 3+i;
        uint32_t h = 4+i;
//        printf("mean i = %d  %dx%d\n",i,w,h);

        compare.setImageSize(w,h);
        compare.gaussian(3);
        compare.gaussian(5);
        compare.gaussian(7);
        compare.gaussian(4);
    }
}

namespace {
    /// Brute force mean of the region around (x,y), reading pixels through the border, in double
    template<class E>
    double naive_mean_border( const ImageBorder<E>& border, int32_t x, int32_t y, int32_t rx, int32_t ry ) {
        double sum = 0;
        for (int32_t j = -ry; j <= ry; j++) {
            for (int32_t i = -rx; i <= rx; i++)
                sum += border.get(x + i, y + j);
        }
        return sum/((2*rx + 1)*(2*ry + 1));
    }

    /// Integer division rounded the way BoofCV does it: (total + divisor/2)/divisor with truncation toward zero
    int64_t boofcv_round_div( int64_t total, int64_t divisor ) {
        return (total + divisor/2)/divisor;
    }

    /// Integer mean_border() computed directly from BoofCV's definition: a rounded horizontal mean through the
    /// border, then a rounded vertical mean of that through the border
    template<class E>
    Gray<E> naive_mean_border_integer( const Gray<E>& input, BorderType type, int32_t rx, int32_t ry ) {
        Gray<E> horizontal(input.width, input.height), output(input.width, input.height);
        auto border = FactoryImageBorder::create_SB<E>(type);
        border->setImage(input);
        for (int32_t y = 0; y < (int32_t)input.height; y++) {
            for (int32_t x = 0; x < (int32_t)input.width; x++) {
                int64_t sum = 0;
                for (int32_t i = -rx; i <= rx; i++)
                    sum += border->get(x + i, y);
                horizontal.at(x, y) = (E)boofcv_round_div(sum, 2*rx + 1);
            }
        }
        border->setImage(horizontal);
        for (int32_t y = 0; y < (int32_t)input.height; y++) {
            for (int32_t x = 0; x < (int32_t)input.width; x++) {
                int64_t sum = 0;
                for (int32_t j = -ry; j <= ry; j++)
                    sum += border->get(x, y + j);
                output.at(x, y) = (E)boofcv_round_div(sum, 2*ry + 1);
            }
        }
        return output;
    }

    /// Compares mean_border() against the brute force mean for every border type and a sub-image. Floats are
    /// compared to the mean in double, integers must exactly match the rounded two pass mean.
    template<class E>
    void mean_border_compare_to_naive( uint32_t width, uint32_t height, int32_t rx, int32_t ry, E min_value,
                                       E max_value ) {
        Gray<E> input(width, height);
        std::mt19937 gen(0xBEEF + width*31 + height);
        ImageMiscOps::fill_uniform(input, min_value, max_value, gen);

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<E> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<E>(type);
                Gray<E> output(1, 1), storage(1, 1);
                std::vector<typename TypeInfo<E>::sum_type> totals;
                BlurImageOps::mean_border(in, output, rx, ry, *border, storage, totals);

                ASSERT_EQ(width, output.width);
                ASSERT_EQ(height, output.height);
                if constexpr (std::is_floating_point<E>::value) {
                    auto expected_border = FactoryImageBorder::create_SB<E>(type);
                    expected_border->setImage(input);
                    for (int32_t y = 0; y < (int32_t)height; y++) {
                        for (int32_t x = 0; x < (int32_t)width; x++) {
                            ASSERT_NEAR(naive_mean_border(*expected_border, x, y, rx, ry), output.at(x, y), 1e-5)
                                    << width << "x" << height << " r=" << rx << "," << ry << " border=" << type
                                    << " sub=" << sub << " at " << x << "," << y;
                        }
                    }
                } else {
                    Gray<E> expected = naive_mean_border_integer(input, type, rx, ry);
                    for (int32_t y = 0; y < (int32_t)height; y++) {
                        for (int32_t x = 0; x < (int32_t)width; x++) {
                            ASSERT_EQ((int64_t)expected.at(x, y), (int64_t)output.at(x, y))
                                    << width << "x" << height << " r=" << rx << "," << ry << " border=" << type
                                    << " sub=" << sub << " at " << x << "," << y;
                        }
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }

    /// The inner image, the border, and images smaller than the region
    template<class E>
    void mean_border_compare_to_naive( E min_value, E max_value ) {
        for (auto shape : {std::pair{30u, 25u}, {8u, 9u}, {5u, 3u}, {2u, 7u}, {1u, 1u}}) {
            mean_border_compare_to_naive<E>(shape.first, shape.second, 1, 1, min_value, max_value);
            mean_border_compare_to_naive<E>(shape.first, shape.second, 3, 2, min_value, max_value);
            mean_border_compare_to_naive<E>(shape.first, shape.second, 2, 4, min_value, max_value);
        }
    }
}

TEST(BlurImageOps, mean_border_compare_to_naive_U8) {
    mean_border_compare_to_naive<U8>(0, 255);
}

TEST(BlurImageOps, mean_border_compare_to_naive_S16) {
    // negative values check the rounding of negative sums
    mean_border_compare_to_naive<S16>(-30000, 30000);
}

TEST(BlurImageOps, mean_border_compare_to_naive_U16) {
    mean_border_compare_to_naive<U16>(0, 65535);
}

TEST(BlurImageOps, mean_border_compare_to_naive_F32) {
    mean_border_compare_to_naive<F32>(0.0f, 10.0f);
}

TEST(BlurImageOps, mean_border_compare_to_naive_F64) {
    mean_border_compare_to_naive<F64>(0.0, 10.0);
}

TEST(BlurImageOps, mean_border_bad_radius) {
    Gray<F32> input(10, 8), output(1, 1), storage(1, 1);
    std::vector<F32> totals;
    auto border = FactoryImageBorder::create_SB<F32>(REFLECT);
    EXPECT_THROW(BlurImageOps::mean_border(input, output, 0, 2, *border, storage, totals), invalid_argument);
    EXPECT_THROW(BlurImageOps::mean_border(input, output, 2, 0, *border, storage, totals), invalid_argument);
}
