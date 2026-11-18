#include "gtest/gtest.h"
#include "testing_utils.h"

// Strict runs every check
#include "variant_strict.h"
#include "shared/stereo_disparity_checks.h"

using namespace stereo_disparity_checks;

class FactoryStereoDisparityTest : public boofcv::BoofCPPTest {};
class FactoryCensusTransformTest : public boofcv::BoofCPPTest {};
class StereoDisparityBlockMatchTest : public boofcv::BoofCPPTest {};

TEST_F(FactoryStereoDisparityTest, block_match_same_as_hand_built) {
    factory_stereo_disparity_block_match_same_as_hand_built();
}
TEST_F(FactoryStereoDisparityTest, block_match_census_same_as_hand_built) {
    factory_stereo_disparity_block_match_census_same_as_hand_built();
}
TEST_F(FactoryStereoDisparityTest, block_match_census_F32_same_as_hand_built) {
    factory_stereo_disparity_block_match_census_F32_same_as_hand_built();
}
TEST_F(FactoryStereoDisparityTest, block_match_errors_same_as_hand_built_U8) {
    factory_stereo_disparity_block_match_errors_same_as_hand_built_U8();
}
TEST_F(FactoryStereoDisparityTest, block_match_errors_same_as_hand_built_F32) {
    factory_stereo_disparity_block_match_errors_same_as_hand_built_F32();
}
TEST_F(FactoryStereoDisparityTest, block_match_zssd_ncc_zero_radius) {
    factory_stereo_disparity_block_match_zssd_ncc_zero_radius();
}
TEST_F(FactoryCensusTransformTest, variant) { factory_census_transform_variant(); }
TEST_F(FactoryCensusTransformTest, samples) { factory_census_transform_samples(); }
TEST_F(FactoryCensusTransformTest, variant_skip_border) { factory_census_transform_variant_skip_border(); }
TEST_F(FactoryStereoDisparityTest, block_match_unsupported) { factory_stereo_disparity_block_match_unsupported(); }
TEST_F(StereoDisparityBlockMatchTest, getters) { stereo_disparity_block_match_getters(); }
TEST_F(StereoDisparityBlockMatchTest, score_enabled) { stereo_disparity_block_match_score_enabled(); }
TEST_F(StereoDisparityBlockMatchTest, basic_select_score_not_written) {
    stereo_disparity_block_match_basic_select_score_not_written();
}
TEST_F(StereoDisparityBlockMatchTest, caller_owned_images_must_match_shape) {
    stereo_disparity_block_match_caller_owned_images_must_match_shape();
}
