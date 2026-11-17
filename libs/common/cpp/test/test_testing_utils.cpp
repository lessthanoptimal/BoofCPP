#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "testing_utils.h"

using namespace boofcv;

// EXPECT_FATAL_FAILURE can't use local variables, so each case is a function which compares 1 pixel images

template<class E>
void compare( double expected, double found, double tol, double scale = 1.0 ) {
    Gray<E> a(1, 1), b(1, 1);
    a.at(0, 0) = (E)expected;
    b.at(0, 0) = (E)found;
    check_within(a, b, tol, scale);
}

void f64_pass() { compare<F64>(1000.0, 1000.0 + 0.9e-12, 1e-12); }
void f64_fail() { compare<F64>(1000.0, 1000.0 + 1.1e-12, 1e-12); }
void f64_scaled_pass() { compare<F64>(1000.0, 1000.0 + 0.9e-9, 1e-12, 1000.0); }
void f64_scaled_fail() { compare<F64>(1000.0, 1000.0 + 1.1e-9, 1e-12, 1000.0); }
void f32_zero_tol_fail() { compare<F32>(10.0, 10.000001, 0.0); }
void u8_fail() { compare<U8>(10, 11, float_tolerance<U8>()); }
void size_fail() { Gray<F32> a(2, 1), b(1, 1); check_within(a, b, 1.0); }

class testing_utilsTest : public boofcv::BoofCPPTest {};

TEST_F(testing_utilsTest, check_within) {
    // Absolute, unless scaled
    f64_pass();
    EXPECT_FATAL_FAILURE(f64_fail(), "");
    f64_scaled_pass();
    EXPECT_FATAL_FAILURE(f64_scaled_fail(), "");
    // Zero tolerance and integers require identical values
    compare<F32>(10.0, 10.0, 0.0);
    EXPECT_FATAL_FAILURE(f32_zero_tol_fail(), "");
    compare<U8>(10, 10, float_tolerance<U8>());
    EXPECT_FATAL_FAILURE(u8_fail(), "");
    // Shapes must match
    EXPECT_FATAL_FAILURE(size_fail(), "");
}

TEST_F(testing_utilsTest, float_tolerance) {
    EXPECT_EQ(1e-5, float_tolerance<F32>());
    EXPECT_EQ(1e-12, float_tolerance<F64>());
    EXPECT_EQ(0.0, float_tolerance<U8>());
    EXPECT_EQ(0.0, float_tolerance<S32>());
}
