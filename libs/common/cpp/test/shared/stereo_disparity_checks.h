#ifndef BOOFCPP_STEREO_DISPARITY_CHECKS_H
#define BOOFCPP_STEREO_DISPARITY_CHECKS_H

/// Checks of tested's FactoryStereoDisparity, StereoDisparityBlockMatch and FactoryCensusTransform, shared by every
/// variant. Include a variant header, e.g. variant_strict.h, first. Each variant's tests call every check. The
/// factory's block matcher must be identical to the same pipeline built by hand from tested's parts. The input of ZSSD
/// and NCC is prepared with strict's ImageNormalization and convert()

#include "boofcv/census_variants.h"
#include "boofcv/config_disparity.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

namespace stereo_disparity_checks {
    using namespace std;
    using namespace boofcv;

    /// Stereo pair of type E. F32 images get a fractional part so that floating point rounding matters
    template<class E>
    void create_pair( std::mt19937& rng, Gray<E>& left, Gray<E>& right ) {
        Gray<U8> left8(1, 1), right8(1, 1);
        create_stereo_pair(rng, left8, right8, 30, 20, 3);
        left.reshape(30, 20);
        right.reshape(30, 20);
        for (uint32_t i = 0; i < left8.data_length; i++) {
            left.data[i] = (E)left8.data[i];
            right.data[i] = (E)right8.data[i];
            if constexpr (std::is_floating_point<E>::value) {
                left.data[i] += 0.1f*(F32)(i%7);
                right.data[i] += 0.1f*(F32)(i%5);
            }
        }
    }

    /// Same values, where NaN equals NaN
    inline void check_equals_nan( const Gray<F32>& expected, const Gray<F32>& found ) {
        ASSERT_EQ(expected.width, found.width);
        ASSERT_EQ(expected.height, found.height);
        for (uint32_t y = 0; y < expected.height; y++) {
            for (uint32_t x = 0; x < expected.width; x++) {
                F32 a = expected.at(x, y), b = found.at(x, y);
                if (std::isnan(a))
                    ASSERT_TRUE(std::isnan(b)) << x << " " << y;
                else
                    ASSERT_EQ(a, b) << x << " " << y;
            }
        }
    }

    inline ConfigDisparityBM create_sad_config( bool subpixel ) {
        ConfigDisparityBM config;
        config.error_type = DisparityError::SAD;
        config.disparity_min = -2;
        config.disparity_range = 12;
        config.region_radius_x = 2;
        config.region_radius_y = 1;
        config.max_per_pixel_error = 30;
        config.subpixel = subpixel;
        return config;
    }

    /// Builds the same pipeline by hand and compares the results
    template<class D>
    void compare_to_hand_built( std::mt19937& rng, const ConfigDisparityBM& config ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(rng, left, right, 30, 20, 3);

        auto alg = tested::FactoryStereoDisparity::block_match<U8, D>(config);
        alg->process(left, right);

        double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*config.max_per_pixel_error;
        auto select = make_shared<tested::SelectErrorWithChecks_S32<D>>(max_error, config.validate_r_to_l,
                                                                         config.texture);
        tested::DisparityBlockMatchRowFormat<U8, int32_t> expected(config.region_radius_x, config.region_radius_y,
                                                           make_shared<tested::BlockRowScoreSad<U8>>(), select);
        expected.configure(config.disparity_min, config.disparity_range);
        expected.set_border(config.border);
        Gray<D> disparity(30, 20);
        Gray<F32> score(30, 20);
        select->set_output(disparity, &score);
        expected.process(left, right);

        ASSERT_NE(nullptr, alg->get_score());
        check_equals(disparity, alg->get_disparity(), (D)0);
        check_equals(score, *alg->get_score(), 0.0f);
    }

    inline void factory_stereo_disparity_block_match_same_as_hand_built( std::mt19937& rng ) {
        compare_to_hand_built<U8>(rng, create_sad_config(false));
        compare_to_hand_built<F32>(rng, create_sad_config(true));
    }

    /// Census transform both images with the configuration's border, then block matching on the census images
    /// with the same border
    template<class C, class D, class E = U8>
    void compare_census_to_hand_built( std::mt19937& rng, CensusVariants variant, BorderType border ) {
        Gray<E> left(1, 1), right(1, 1);
        create_pair(rng, left, right);

        ConfigDisparityBM config = create_sad_config(std::is_same<D, F32>::value);
        config.error_type = DisparityError::CENSUS;
        config.config_census.variant = variant;
        config.max_per_pixel_error = 3;
        config.border = border;
        auto alg = tested::FactoryStereoDisparity::block_match<E, D>(config);
        alg->process(left, right);

        auto census_border = FactoryImageBorder::create_SB<E>(border);
        Gray<C> census_left(1, 1), census_right(1, 1);
        if constexpr (std::is_same<C, U8>::value) {
            tested::CensusTransform::dense3x3(left, census_left, census_border.get());
            tested::CensusTransform::dense3x3(right, census_right, census_border.get());
        } else if constexpr (std::is_same<C, S32>::value) {
            tested::CensusTransform::dense5x5(left, census_left, census_border.get());
            tested::CensusTransform::dense5x5(right, census_right, census_border.get());
        } else {
            auto samples = tested::FactoryCensusTransform::samples(variant);
            tested::CensusTransform::sample_S64(left, samples, census_left, census_border.get());
            tested::CensusTransform::sample_S64(right, samples, census_right, census_border.get());
        }

        double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*config.max_per_pixel_error;
        auto select = make_shared<tested::SelectErrorWithChecks_S32<D>>(max_error, config.validate_r_to_l,
                                                                         config.texture);
        tested::DisparityBlockMatchRowFormat<C, int32_t> expected(
                config.region_radius_x, config.region_radius_y,
                make_shared<tested::BlockRowScoreCensus<C>>(census_bits(variant)), select);
        expected.configure(config.disparity_min, config.disparity_range);
        expected.set_border(config.border);
        Gray<D> disparity(30, 20);
        Gray<F32> score(30, 20);
        select->set_output(disparity, &score);
        expected.process(census_left, census_right);

        ASSERT_NE(nullptr, alg->get_score());
        check_equals(disparity, alg->get_disparity(), (D)0);
        check_equals(score, *alg->get_score(), 0.0f);

        auto& census = dynamic_cast<const tested::StereoDisparityBlockMatchCensus<E, C, D>&>(*alg);
        check_equals(census_left, census.get_census_left(), (C)0);
        check_equals(census_right, census.get_census_right(), (C)0);
    }

    inline void factory_stereo_disparity_block_match_census_same_as_hand_built( std::mt19937& rng ) {
        compare_census_to_hand_built<U8, U8>(rng, CensusVariants::BLOCK_3_3, REFLECT);
        compare_census_to_hand_built<U8, F32>(rng, CensusVariants::BLOCK_3_3, REFLECT);
        compare_census_to_hand_built<S32, U8>(rng, CensusVariants::BLOCK_5_5, REFLECT);
        compare_census_to_hand_built<S32, F32>(rng, CensusVariants::BLOCK_5_5, REFLECT);
        compare_census_to_hand_built<S32, F32>(rng, CensusVariants::BLOCK_5_5, ZERO);
        for (CensusVariants variant : {CensusVariants::BLOCK_7_7, CensusVariants::BLOCK_9_7, CensusVariants::BLOCK_13_5,
                                       CensusVariants::CIRCLE_9}) {
            compare_census_to_hand_built<S64, U8>(rng, variant, REFLECT);
            compare_census_to_hand_built<S64, F32>(rng, variant, ZERO);
        }
    }

    inline void factory_stereo_disparity_block_match_census_F32_same_as_hand_built( std::mt19937& rng ) {
        compare_census_to_hand_built<U8, F32, F32>(rng, CensusVariants::BLOCK_3_3, REFLECT);
        compare_census_to_hand_built<S32, U8, F32>(rng, CensusVariants::BLOCK_5_5, ZERO);
        compare_census_to_hand_built<S64, F32, F32>(rng, CensusVariants::CIRCLE_9, EXTENDED);
    }

    /// Builds the pipeline for SAD, SSD, ZSSD, or NCC by hand and compares the results
    template<class E, class D>
    void compare_error_to_hand_built( std::mt19937& rng, DisparityError error, bool normalize_input,
                                      BorderType border ) {
        Gray<E> left(1, 1), right(1, 1);
        create_pair(rng, left, right);

        ConfigDisparityBM config = create_sad_config(std::is_same<D, F32>::value);
        config.error_type = error;
        config.border = border;
        config.max_per_pixel_error = error == DisparityError::SAD ? 30 : 900;
        config.config_ncc.normalize_input = normalize_input;
        config.config_ncc.eps = 1e-3;
        config.catastrophic_reset = 4;
        auto alg = tested::FactoryStereoDisparity::block_match<E, D>(config);
        alg->process(left, right);

        const double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*
                                 config.max_per_pixel_error;
        const bool squared = error != DisparityError::SAD;
        Gray<D> disparity(30, 20);
        Gray<F32> score(30, 20);

        if (error == DisparityError::SAD || error == DisparityError::SSD) {
            using S = tested::BlockScoreType<E>;
            auto select = make_shared<tested::SelectErrorWithChecks<S, D>>(
                    max_error, config.validate_r_to_l, config.texture, squared);
            std::shared_ptr<tested::BlockRowScore<E, S>> row_score;
            if (error == DisparityError::SAD)
                row_score = make_shared<tested::BlockRowScoreSad<E>>();
            else
                row_score = make_shared<tested::BlockRowScoreSsd<E>>();
            tested::DisparityBlockMatchRowFormat<E, S> expected(config.region_radius_x, config.region_radius_y,
                                                        row_score, select);
            expected.configure(config.disparity_min, config.disparity_range);
            expected.set_border(config.border);
            expected.set_catastrophic_reset(config.catastrophic_reset);
            select->set_output(disparity, &score);
            expected.process(left, right);
        } else {
            Gray<F32> adjusted_left(1, 1), adjusted_right(1, 1);
            if (error == DisparityError::NCC && normalize_input) {
                boofcv::NormalizeParameters parameters;
                boofcv::ImageNormalization::zero_mean_max_one(left, adjusted_left, parameters);
                boofcv::ImageNormalization::apply(right, parameters, adjusted_right);
            } else {
                boofcv::convert(left, adjusted_left);
                boofcv::convert(right, adjusted_right);
            }

            std::shared_ptr<tested::BlockRowScore<F32, F32>> row_score;
            std::shared_ptr<tested::DisparitySelect<F32>> select;
            if (error == DisparityError::NCC) {
                auto s = make_shared<tested::SelectCorrelationWithChecks_F32<D>>(config.validate_r_to_l,
                                                                                 config.texture);
                s->set_output(disparity, &score);
                select = s;
                row_score = make_shared<tested::BlockRowScoreNcc>(config.region_radius_x, config.region_radius_y,
                                                          config.border, (F32)config.config_ncc.eps);
            } else {
                auto s = make_shared<tested::SelectErrorWithChecks_F32<D>>(
                        max_error, config.validate_r_to_l, config.texture, true);
                s->set_output(disparity, &score);
                select = s;
                row_score = make_shared<tested::BlockRowScoreZssd>(config.region_radius_x, config.region_radius_y,
                                                           config.border);
            }
            tested::DisparityBlockMatchRowFormat<F32, F32> expected(config.region_radius_x, config.region_radius_y,
                                                            row_score, select);
            expected.configure(config.disparity_min, config.disparity_range);
            expected.set_border(config.border);
            expected.set_catastrophic_reset(config.catastrophic_reset);
            expected.process(adjusted_left, adjusted_right);

            auto& pre = dynamic_cast<const tested::StereoDisparityBlockMatchPreFilter<E, D>&>(*alg);
            check_equals(adjusted_left, pre.get_adjusted_left(), 0.0f);
            check_equals(adjusted_right, pre.get_adjusted_right(), 0.0f);
        }

        ASSERT_NE(nullptr, alg->get_score());
        check_equals(disparity, alg->get_disparity(), (D)0);
        check_equals_nan(score, *alg->get_score());

        // make sure the comparison isn't trivial
        uint32_t valid = 0;
        for (uint32_t i = 0; i < disparity.data_length; i++) {
            if (disparity.data[i] < (D)config.disparity_range)
                valid++;
        }
        ASSERT_GT(valid, disparity.data_length/4) << DISPARITY_ERROR_NAMES[(int)error];
    }

    template<class E>
    void compare_every_error_to_hand_built( std::mt19937& rng ) {
        for (DisparityError error : {DisparityError::SAD, DisparityError::SSD, DisparityError::ZSSD,
                                     DisparityError::NCC}) {
            compare_error_to_hand_built<E, U8>(rng, error, true, REFLECT);
            compare_error_to_hand_built<E, F32>(rng, error, true, REFLECT);
            compare_error_to_hand_built<E, F32>(rng, error, true, ZERO);
        }
        compare_error_to_hand_built<E, F32>(rng, DisparityError::NCC, false, REFLECT);
    }

    inline void factory_stereo_disparity_block_match_errors_same_as_hand_built_U8( std::mt19937& rng ) {
        compare_every_error_to_hand_built<U8>(rng);
    }

    inline void factory_stereo_disparity_block_match_errors_same_as_hand_built_F32( std::mt19937& rng ) {
        compare_every_error_to_hand_built<F32>(rng);
    }

    /// The local statistics of ZSSD and NCC need a region radius of at least 1, same as BoofCV
    inline void factory_stereo_disparity_block_match_zssd_ncc_zero_radius( std::mt19937& rng ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_pair(rng, left, right);
        for (DisparityError error : {DisparityError::ZSSD, DisparityError::NCC}) {
            ConfigDisparityBM config = create_sad_config(true);
            config.error_type = error;
            config.region_radius_x = 0;
            auto alg = tested::FactoryStereoDisparity::block_match<U8, F32>(config);
            EXPECT_THROW(alg->process(left, right), invalid_argument);
        }
    }

    inline void factory_census_transform_variant() {
        EXPECT_NE(nullptr, (tested::FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, REFLECT)));
        EXPECT_NE(nullptr, (tested::FactoryCensusTransform::variant<U8, S32>(CensusVariants::BLOCK_5_5, REFLECT)));
        // census image type must match the variant
        EXPECT_THROW((tested::FactoryCensusTransform::variant<U8, S32>(CensusVariants::BLOCK_3_3, REFLECT)),
                     invalid_argument);
        EXPECT_THROW((tested::FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_5_5, REFLECT)),
                     invalid_argument);
        for (CensusVariants variant : {CensusVariants::BLOCK_7_7, CensusVariants::BLOCK_9_7, CensusVariants::BLOCK_13_5,
                                       CensusVariants::CIRCLE_9}) {
            EXPECT_NE(nullptr, (tested::FactoryCensusTransform::variant<U8, S64>(variant, REFLECT)));
            EXPECT_THROW((tested::FactoryCensusTransform::variant<U8, S32>(variant, REFLECT)), invalid_argument);
        }
        EXPECT_THROW((tested::FactoryCensusTransform::variant<U8, S64>(CensusVariants::BLOCK_5_5, REFLECT)),
                     invalid_argument);
        EXPECT_THROW((tested::FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, NORMALIZED)),
                     invalid_argument);
    }

    /// Each S64 variant samples the block or circle it's named after, one point per bit
    inline void factory_census_transform_samples() {
        struct Expected { CensusVariants variant; int32_t radius_x, radius_y; };
        for (Expected e : {Expected{CensusVariants::BLOCK_7_7, 3, 3}, {CensusVariants::BLOCK_9_7, 4, 3},
                           {CensusVariants::BLOCK_13_5, 6, 2}, {CensusVariants::CIRCLE_9, 4, 4}}) {
            auto samples = tested::FactoryCensusTransform::samples(e.variant);
            ASSERT_EQ(census_bits(e.variant), samples.size());
            int32_t max_x = 0, max_y = 0;
            for (const auto& p : samples) {
                max_x = std::max(max_x, std::abs(p.x));
                max_y = std::max(max_y, std::abs(p.y));
            }
            EXPECT_EQ(e.radius_x, max_x);
            EXPECT_EQ(e.radius_y, max_y);
        }
        EXPECT_THROW(tested::FactoryCensusTransform::samples(CensusVariants::BLOCK_5_5), invalid_argument);
    }

    /// SKIP leaves the image border untouched, same as BoofCV
    inline void factory_census_transform_variant_skip_border( std::mt19937& rng ) {
        Gray<U8> input(12, 10);
        ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, rng);

        Gray<U8> expected(12, 10), found(12, 10);
        ImageMiscOps::fill(expected, (U8)7);
        ImageMiscOps::fill(found, (U8)7);
        tested::CensusTransform::dense3x3(input, expected, (ImageBorder<U8>*)nullptr);
        tested::FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, SKIP)->process(input, found);
        check_equals(expected, found, (U8)0);
    }

    inline void factory_stereo_disparity_block_match_unsupported() {
        // subpixel and the disparity type must agree
        EXPECT_THROW((tested::FactoryStereoDisparity::block_match<U8, U8>(create_sad_config(true))), invalid_argument);
        EXPECT_THROW((tested::FactoryStereoDisparity::block_match<U8, F32>(create_sad_config(false))),
                     invalid_argument);
        // only U8 and F32 images
        EXPECT_THROW((tested::FactoryStereoDisparity::block_match<U16, F32>(create_sad_config(true))),
                     invalid_argument);
        // invalid configuration
        ConfigDisparityBM config = create_sad_config(true);
        config.disparity_range = 0;
        EXPECT_THROW((tested::FactoryStereoDisparity::block_match<U8, F32>(config)), invalid_argument);
    }

    inline void stereo_disparity_block_match_getters() {
        auto alg = tested::FactoryStereoDisparity::block_match<U8, F32>(create_sad_config(true));
        EXPECT_EQ(-2, alg->get_disparity_min());
        EXPECT_EQ(12u, alg->get_disparity_range());
        EXPECT_EQ(12, alg->get_invalid_value());
        EXPECT_EQ(0, alg->get_border_x());
        EXPECT_EQ(1, alg->get_border_y());
    }

    inline void stereo_disparity_block_match_score_enabled( std::mt19937& rng ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(rng, left, right, 30, 20, 3);
        ConfigDisparityBM config = create_sad_config(false);
        config.save_score = false;
        auto alg = tested::FactoryStereoDisparity::block_match<U8, U8>(config);
        alg->process(left, right);
        EXPECT_EQ(nullptr, alg->get_score());

        alg->set_score_enabled(true);
        alg->process(left, right);
        ASSERT_NE(nullptr, alg->get_score());
        EXPECT_EQ(30u, alg->get_score()->width);
        EXPECT_EQ(20u, alg->get_score()->height);
    }

    /// With every check off BoofCV's selector doesn't write the score, but it's still allocated
    inline void stereo_disparity_block_match_basic_select_score_not_written( std::mt19937& rng ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(rng, left, right, 30, 20, 3);
        ConfigDisparityBM config = create_sad_config(false);
        config.max_per_pixel_error = -1;
        config.validate_r_to_l = -1;
        config.texture = 0;
        auto alg = tested::FactoryStereoDisparity::block_match<U8, U8>(config);
        alg->process(left, right);

        ASSERT_NE(nullptr, alg->get_score());
        uint32_t valid = 0;
        for (uint32_t y = 0; y < 20; y++) {
            for (uint32_t x = 0; x < 30; x++) {
                ASSERT_EQ(0.0f, alg->get_score()->at(x, y));
                if (alg->get_disparity().at(x, y) < 12)
                    valid++;
            }
        }
        EXPECT_GT(valid, 0u);
    }

    inline void stereo_disparity_block_match_caller_owned_images_must_match_shape( std::mt19937& rng ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(rng, left, right, 30, 20, 3);
        auto alg = tested::FactoryStereoDisparity::block_match<U8, U8>(create_sad_config(false));
        Gray<U8> disparity(29, 20);
        EXPECT_THROW(alg->process(left, right, disparity, nullptr), invalid_argument);
        Gray<U8> disparity_ok(30, 20);
        Gray<F32> score(30, 19);
        EXPECT_THROW(alg->process(left, right, disparity_ok, &score), invalid_argument);
    }
}

#endif
