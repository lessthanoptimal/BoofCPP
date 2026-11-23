#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/mean_divisor.h"

#include <cmath>
#include <cstdint>
#include <limits>

using namespace std;
using namespace boofcv;

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
