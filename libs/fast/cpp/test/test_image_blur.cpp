#include "gtest/gtest.h"
#include "testing_utils.h"

// Fast's mean, with and without a border, supports F32 and F64
#include "variant_fast.h"
#include "shared/blur_mean_checks.h"

using namespace boofcv;

class BlurImageOpsTest : public boofcv::BoofCPPTest {};

TEST_F(BlurImageOpsTest, mean_F32) { blur_mean_checks::check_mean_all<F32>(rng); }
TEST_F(BlurImageOpsTest, mean_F64) { blur_mean_checks::check_mean_all<F64>(rng); }
TEST_F(BlurImageOpsTest, mean_bad_radius) { blur_mean_checks::check_mean_bad_radius<F32>(); }
TEST_F(BlurImageOpsTest, mean_with_border_F32) { blur_mean_checks::check_mean_with_border_all<F32>(rng, 0, 10); }
TEST_F(BlurImageOpsTest, mean_with_border_F64) { blur_mean_checks::check_mean_with_border_all<F64>(rng, 0, 10); }
TEST_F(BlurImageOpsTest, mean_with_border_bad_radius) {
    blur_mean_checks::check_mean_with_border_bad_radius<F32>();
}
