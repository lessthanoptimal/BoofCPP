#include "gtest/gtest.h"
#include "testing_utils.h"

// Fast's mean supports F32 and F64
#include "variant_fast.h"
#include "shared/blur_mean_checks.h"

using namespace boofcv;

class BlurImageOpsTest : public boofcv::BoofCPPTest {};

TEST_F(BlurImageOpsTest, mean_F32) { blur_mean_checks::check_mean_all<F32>(); }
TEST_F(BlurImageOpsTest, mean_F64) { blur_mean_checks::check_mean_all<F64>(); }

TEST_F(BlurImageOpsTest, mean_bad_radius) {
    Gray<F32> input(10, 8), output(10, 8), storage(1, 1);
    EXPECT_THROW(fast::BlurImageOps::mean(input, output, 0, 2, storage), std::invalid_argument);
    EXPECT_THROW(fast::BlurImageOps::mean(input, output, 2, 0, storage), std::invalid_argument);
}

/// A radius larger than the image covers the same pixels as one the size of the image
TEST_F(BlurImageOpsTest, mean_huge_radius) {
    std::mt19937 gen(0xBEEF);
    Gray<F32> input(10, 8), expected(1, 1), found(1, 1), storage(1, 1);
    ImageMiscOps::fill_uniform(input, 0.0f, 50.0f, gen);
    fast::BlurImageOps::mean(input, expected, 10, 8, storage);
    fast::BlurImageOps::mean(input, found, 0x7FFFFFFF, 0x7FFFFFFF, storage);
    check_within(expected, found, 0.0);
}
