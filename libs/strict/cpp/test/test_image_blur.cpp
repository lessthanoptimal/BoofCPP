#include "gtest/gtest.h"
#include "boofcv/image_blur.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <random>

#include "boofcv/print_structures.h"

using namespace std;
using namespace boofcv;

// BlurImageOps::mean, mean_border and gaussian are tested by checks shared with the other variants
#include "variant_strict.h"
#include "shared/blur_mean_checks.h"
#include "shared/blur_mean_border_checks.h"
#include "shared/blur_gaussian_checks.h"

class BlurImageOpsTest : public boofcv::BoofCPPTest {};

TEST_F(BlurImageOpsTest, mean_U8) { blur_mean_checks::check_mean_all<U8>(); }
TEST_F(BlurImageOpsTest, mean_U16) { blur_mean_checks::check_mean_all<U16>(); }
TEST_F(BlurImageOpsTest, mean_F32) { blur_mean_checks::check_mean_all<F32>(); }
TEST_F(BlurImageOpsTest, mean_F64) { blur_mean_checks::check_mean_all<F64>(); }
TEST_F(BlurImageOpsTest, mean_bad_radius) { blur_mean_checks::check_mean_bad_radius<F32>(); }

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

class MeanDivisorTest : public boofcv::BoofCPPTest {};

TEST_F(MeanDivisorTest, U8) {
    for (uint32_t length = 1; length <= 101; length++)
        check_mean_divisor<U8>(length);
    // 8405024 is the largest length that uses a multiplication, 8405025 divides
    for (uint32_t length : {255u, 256u, 1001u, 65537u, 8405024u, 8405025u})
        check_mean_divisor<U8>(length);
}

TEST_F(MeanDivisorTest, U16) {
    for (uint32_t length = 1; length <= 61; length++)
        check_mean_divisor<U16>(length);
    // 32768 is the largest length that uses a multiplication, 32769 divides
    for (uint32_t length : {255u, 1001u, 32768u, 32769u, 65535u})
        check_mean_divisor<U16>(length);
}

/// The 32-bit multiplication is used when the products fit, e.g. U8 Gaussian kernels, but not for U16
TEST_F(MeanDivisorTest, multiply32) {
    ASSERT_TRUE(MeanDivisor<U8>(71).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U8>(11).uses_multiply32());
    ASSERT_FALSE(MeanDivisor<U8>(8405024).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U8>(8405024).uses_multiply());
    ASSERT_FALSE(MeanDivisor<U16>(71).uses_multiply32());
    ASSERT_TRUE(MeanDivisor<U16>(71).uses_multiply());
}

TEST_F(MeanDivisorTest, signed_and_float) {
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

class ConvolveImageMeanTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveImageMeanTest, inner_horizontal_U8) {
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

TEST_F(ConvolveImageMeanTest, inner_horizontal_F32) {
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

TEST_F(ConvolveImageMeanTest, inner_vertical_U8) {
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

TEST_F(ConvolveImageMeanTest, inner_vertical_F32) {
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

TEST_F(ConvolveImageMeanTest, horizonal) {
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

TEST_F(ConvolveImageMeanTest, vertical) {
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

TEST_F(BlurImageOpsTest, gaussian_U8) { blur_gaussian_checks::check_all<U8>(); }
TEST_F(BlurImageOpsTest, gaussian_U16) { blur_gaussian_checks::check_all<U16>(); }
TEST_F(BlurImageOpsTest, gaussian_F32) { blur_gaussian_checks::check_all<F32>(); }
TEST_F(BlurImageOpsTest, gaussian_F64) { blur_gaussian_checks::check_all<F64>(); }

TEST_F(BlurImageOpsTest, mean_border_U8) { blur_mean_border_checks::check_all<U8>(0, 255); }
// negative values check the rounding of negative sums
TEST_F(BlurImageOpsTest, mean_border_S16) { blur_mean_border_checks::check_all<S16>(-30000, 30000); }
TEST_F(BlurImageOpsTest, mean_border_U16) { blur_mean_border_checks::check_all<U16>(0, 65535); }
TEST_F(BlurImageOpsTest, mean_border_F32) { blur_mean_border_checks::check_all<F32>(0, 10); }
TEST_F(BlurImageOpsTest, mean_border_F64) { blur_mean_border_checks::check_all<F64>(0, 10); }
TEST_F(BlurImageOpsTest, mean_border_bad_radius) { blur_mean_border_checks::check_bad_radius<F32>(); }
