#include "gtest/gtest.h"
#include "testing_utils.h"

#include <string>

// Strict runs every check
#include "variant_strict.h"
#include "shared/stereo_disparity_checks.h"
#include "disparity_select_reference.h"

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

namespace {
    /// Pair of type E. A small max_value gives low texture, with many equal scores
    template<class E>
    void create_pair( Gray<E>& left, Gray<E>& right, uint32_t width, uint32_t height, int32_t max_value,
                      uint32_t seed ) {
        Gray<U8> left8(1, 1), right8(1, 1);
        create_stereo_pair(left8, right8, width, height, 3, seed);
        left.reshape(width, height);
        right.reshape(width, height);
        for (uint32_t i = 0; i < left8.data_length; i++) {
            left.data[i] = (E)(left8.data[i]*max_value/255);
            right.data[i] = (E)(right8.data[i]*max_value/255);
            if constexpr (std::is_floating_point<E>::value) {
                left.data[i] += 0.1f*(F32)(i%7);
                right.data[i] += 0.1f*(F32)(i%5);
            }
        }
    }

    /// Block matching against the reference's for one configuration. Results must be identical
    template<class E, class D>
    void compare_to_reference( const ConfigDisparityBM& config, const Gray<E>& left, const Gray<E>& right,
                            const std::string& what ) {
        auto expected = reference::FactoryStereoDisparity::block_match<E, D>(config);
        auto found = boofcv::FactoryStereoDisparity::block_match<E, D>(config);
        expected->process(left, right);
        found->process(left, right);
        check_identical(expected->get_disparity(), found->get_disparity(), what);
        ASSERT_EQ(expected->get_score() == nullptr, found->get_score() == nullptr) << what;
        if (expected->get_score() != nullptr)
            check_identical(*expected->get_score(), *found->get_score(), what);
    }

    /// Every error type, with settings that change which score type and which selector paths are used
    template<class E>
    void compare_every_config_to_reference() {
        // {radius x, radius y}: small, and 9x9 where SAD's largest score doesn't fit 16 bits
        const int32_t radii[][2] = {{1, 1}, {3, 2}, {4, 4}};
        // {min, range}: positive, negative, only negative, a range of 3, and larger than the image
        const int32_t disparities[][2] = {{0, 20}, {4, 9}, {-6, 15}, {-12, 8}, {0, 3}, {0, 60}};
        // {max per pixel error, right to left, texture}: defaults, all off, then each check alone
        const double checks[][3] = {{0, 1, 0.15}, {-1, -1, 0}, {20, -1, 0}, {-1, 0, 0}, {-1, -1, 0.3}};
        const DisparityError errors[] = {DisparityError::SAD, DisparityError::SSD, DisparityError::ZSSD,
                                         DisparityError::NCC, DisparityError::CENSUS};
        const CensusVariants variants[] = {CensusVariants::BLOCK_3_3, CensusVariants::BLOCK_5_5,
                                           CensusVariants::BLOCK_13_5};
        Gray<E> textured_l(1, 1), textured_r(1, 1), flat_l(1, 1), flat_r(1, 1);
        create_pair(textured_l, textured_r, 37, 29, 255, 0xBEEF);
        create_pair(flat_l, flat_r, 37, 29, 3, 0xCAFE);
        for (DisparityError error : errors)
        for (CensusVariants variant : variants)
        for (const auto& radius : radii)
        for (const auto& disparity : disparities)
        for (const auto& check : checks)
        for (bool subpixel : {false, true}) {
            if (error != DisparityError::CENSUS && variant != CensusVariants::BLOCK_3_3)
                continue;
            ConfigDisparityBM config;
            config.error_type = error;
            config.config_census.variant = variant;
            config.region_radius_x = radius[0];
            config.region_radius_y = radius[1];
            config.disparity_min = disparity[0];
            config.disparity_range = disparity[1];
            config.max_per_pixel_error = check[0];
            if (check[0] > 0 && error == DisparityError::CENSUS)
                config.max_per_pixel_error = 3;
            else if (check[0] > 0 && (error == DisparityError::SSD || error == DisparityError::ZSSD))
                config.max_per_pixel_error = check[0]*check[0];
            config.validate_r_to_l = (int32_t)check[1];
            config.texture = check[2];
            config.subpixel = subpixel;
            const std::string what = std::string(DISPARITY_ERROR_NAMES[(int)error]) + " variant " +
                    std::to_string((int)variant) + " r=" + std::to_string(radius[0]) + "," +
                    std::to_string(radius[1]) + " d=" + std::to_string(disparity[0]) + "," +
                    std::to_string(disparity[1]) + " checks=" + std::to_string(check[0]) + "," +
                    std::to_string(check[1]) + "," + std::to_string(check[2]) + " subpixel=" +
                    std::to_string(subpixel);
            for (int flat = 0; flat < 2; flat++) {
                const Gray<E>& l = flat ? flat_l : textured_l;
                const Gray<E>& r = flat ? flat_r : textured_r;
                if (subpixel)
                    compare_to_reference<E, F32>(config, l, r, what + " flat=" + std::to_string(flat));
                else
                    compare_to_reference<E, U8>(config, l, r, what + " flat=" + std::to_string(flat));
                if (::testing::Test::HasFatalFailure())
                    return;
            }
        }
    }
}

/// The selectors and 16-bit scores where they fit give the reference's results
TEST_F(FactoryStereoDisparityTest, block_match_same_as_reference_U8) { compare_every_config_to_reference<U8>(); }
TEST_F(FactoryStereoDisparityTest, block_match_same_as_reference_F32) {
    compare_every_config_to_reference<F32>();
}

/// 16-bit scores are used only when the region's largest score and the range are below 65535
TEST_F(FactoryStereoDisparityTest, small_scores_only_where_they_fit) {
    using Factory = boofcv::FactoryStereoDisparity;
    ConfigDisparityBM config;
    config.error_type = DisparityError::SAD;
    // 15x17 = 255 pixels, 255*255 = 65025 fits. 17x17 = 289 pixels doesn't
    config.region_radius_x = 7;
    config.region_radius_y = 8;
    EXPECT_TRUE(Factory::small_scores(config, 255));
    config.region_radius_x = 8;
    EXPECT_FALSE(Factory::small_scores(config, 255));
    config.region_radius_x = 1;
    config.region_radius_y = 1;
    config.disparity_range = 65535;
    EXPECT_FALSE(Factory::small_scores(config, 255));
    config.disparity_range = 65534;
    EXPECT_TRUE(Factory::small_scores(config, 255));
}
