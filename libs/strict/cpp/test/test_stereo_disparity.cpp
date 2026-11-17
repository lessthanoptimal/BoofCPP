#include "gtest/gtest.h"
#include "boofcv/stereo_disparity.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <random>
#include <set>
#include <string>

using namespace std;
using namespace boofcv;

namespace {
    /// Stereo pair of type E. F32 images get a fractional part so that floating point rounding matters
    template<class E>
    void create_pair( Gray<E>& left, Gray<E>& right ) {
        Gray<U8> left8(1, 1), right8(1, 1);
        create_stereo_pair(left8, right8, 30, 20, 3);
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
    void check_equals_nan( const Gray<F32>& expected, const Gray<F32>& found ) {
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

    ConfigDisparityBM create_sad_config( bool subpixel ) {
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
    void compare_to_hand_built( const ConfigDisparityBM& config ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(left, right, 30, 20, 3);

        auto alg = FactoryStereoDisparity::block_match<U8, D>(config);
        alg->process(left, right);

        double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*config.max_per_pixel_error;
        auto select = make_shared<SelectErrorWithChecks_S32<D>>(max_error, config.validate_r_to_l, config.texture);
        DisparityBlockMatchRowFormat<U8, int32_t> expected(config.region_radius_x, config.region_radius_y,
                                                           make_shared<BlockRowScoreSad<U8>>(), select);
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
}

TEST(ConfigDisparityBM, defaults_same_as_java) {
    ConfigDisparityBM config;
    EXPECT_EQ(0, config.disparity_min);
    EXPECT_EQ(100, config.disparity_range);
    EXPECT_EQ(3, config.region_radius_x);
    EXPECT_EQ(3, config.region_radius_y);
    EXPECT_EQ(0.0, config.max_per_pixel_error);
    EXPECT_EQ(1, config.validate_r_to_l);
    EXPECT_EQ(0.15, config.texture);
    EXPECT_TRUE(config.subpixel);
    EXPECT_TRUE(config.save_score);
    EXPECT_EQ(DisparityError::CENSUS, config.error_type);
    EXPECT_EQ(REFLECT, config.border);
    EXPECT_EQ(25, config.catastrophic_reset);
    EXPECT_EQ(CensusVariants::BLOCK_5_5, config.config_census.variant);
    EXPECT_EQ(5.5e-6, config.config_ncc.eps);
    EXPECT_TRUE(config.config_ncc.normalize_input);
}

TEST(ConfigDisparityBM, visit_fields_unique_names) {
    ConfigDisparityBM config;
    set<string> names;
    uint32_t count = 0;
    visit_fields(config, [&]( const char* name, auto& ) {
        names.insert(name);
        count++;
    });
    EXPECT_EQ(14u, count);
    EXPECT_EQ(count, names.size());
}

TEST(ConfigDisparityBM, check_validity) {
    ConfigDisparityBM config;
    EXPECT_NO_THROW(config.checkValidity());
    config.disparity_range = 0;
    EXPECT_THROW(config.checkValidity(), invalid_argument);
    config.disparity_range = 10;
    for (BorderType type : {NORMALIZED, SKIP}) {
        config.border = type;
        EXPECT_THROW(config.checkValidity(), invalid_argument);
    }
}

TEST(FactoryStereoDisparity, block_match_same_as_hand_built) {
    compare_to_hand_built<U8>(create_sad_config(false));
    compare_to_hand_built<F32>(create_sad_config(true));
}

namespace {
    /// Census transform both images with the configuration's border, then block matching on the census images
    /// with the same border
    template<class C, class D, class E = U8>
    void compare_census_to_hand_built( CensusVariants variant, BorderType border ) {
        Gray<E> left(1, 1), right(1, 1);
        create_pair(left, right);

        ConfigDisparityBM config = create_sad_config(std::is_same<D, F32>::value);
        config.error_type = DisparityError::CENSUS;
        config.config_census.variant = variant;
        config.max_per_pixel_error = 3;
        config.border = border;
        auto alg = FactoryStereoDisparity::block_match<E, D>(config);
        alg->process(left, right);

        auto census_border = FactoryImageBorder::create_SB<E>(border);
        Gray<C> census_left(1, 1), census_right(1, 1);
        if constexpr (std::is_same<C, U8>::value) {
            CensusTransform::dense3x3(left, census_left, census_border.get());
            CensusTransform::dense3x3(right, census_right, census_border.get());
        } else if constexpr (std::is_same<C, S32>::value) {
            CensusTransform::dense5x5(left, census_left, census_border.get());
            CensusTransform::dense5x5(right, census_right, census_border.get());
        } else {
            auto samples = FactoryCensusTransform::samples(variant);
            CensusTransform::sample_S64(left, samples, census_left, census_border.get());
            CensusTransform::sample_S64(right, samples, census_right, census_border.get());
        }

        double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*config.max_per_pixel_error;
        auto select = make_shared<SelectErrorWithChecks_S32<D>>(max_error, config.validate_r_to_l, config.texture);
        DisparityBlockMatchRowFormat<C, int32_t> expected(
                config.region_radius_x, config.region_radius_y,
                make_shared<BlockRowScoreCensus<C>>(census_bits(variant)), select);
        expected.configure(config.disparity_min, config.disparity_range);
        expected.set_border(config.border);
        Gray<D> disparity(30, 20);
        Gray<F32> score(30, 20);
        select->set_output(disparity, &score);
        expected.process(census_left, census_right);

        ASSERT_NE(nullptr, alg->get_score());
        check_equals(disparity, alg->get_disparity(), (D)0);
        check_equals(score, *alg->get_score(), 0.0f);

        auto& census = dynamic_cast<const StereoDisparityBlockMatchCensus<E, C, D>&>(*alg);
        check_equals(census_left, census.get_census_left(), (C)0);
        check_equals(census_right, census.get_census_right(), (C)0);
    }
}

TEST(FactoryStereoDisparity, block_match_census_same_as_hand_built) {
    compare_census_to_hand_built<U8, U8>(CensusVariants::BLOCK_3_3, REFLECT);
    compare_census_to_hand_built<U8, F32>(CensusVariants::BLOCK_3_3, REFLECT);
    compare_census_to_hand_built<S32, U8>(CensusVariants::BLOCK_5_5, REFLECT);
    compare_census_to_hand_built<S32, F32>(CensusVariants::BLOCK_5_5, REFLECT);
    compare_census_to_hand_built<S32, F32>(CensusVariants::BLOCK_5_5, ZERO);
    for (CensusVariants variant : {CensusVariants::BLOCK_7_7, CensusVariants::BLOCK_9_7, CensusVariants::BLOCK_13_5,
                                   CensusVariants::CIRCLE_9}) {
        compare_census_to_hand_built<S64, U8>(variant, REFLECT);
        compare_census_to_hand_built<S64, F32>(variant, ZERO);
    }
}

TEST(FactoryStereoDisparity, block_match_census_F32_same_as_hand_built) {
    compare_census_to_hand_built<U8, F32, F32>(CensusVariants::BLOCK_3_3, REFLECT);
    compare_census_to_hand_built<S32, U8, F32>(CensusVariants::BLOCK_5_5, ZERO);
    compare_census_to_hand_built<S64, F32, F32>(CensusVariants::CIRCLE_9, EXTENDED);
}

namespace {
    /// Builds the pipeline for SAD, SSD, ZSSD, or NCC by hand and compares the results
    template<class E, class D>
    void compare_error_to_hand_built( DisparityError error, bool normalize_input, BorderType border ) {
        Gray<E> left(1, 1), right(1, 1);
        create_pair(left, right);

        ConfigDisparityBM config = create_sad_config(std::is_same<D, F32>::value);
        config.error_type = error;
        config.border = border;
        config.max_per_pixel_error = error == DisparityError::SAD ? 30 : 900;
        config.config_ncc.normalize_input = normalize_input;
        config.config_ncc.eps = 1e-3;
        config.catastrophic_reset = 4;
        auto alg = FactoryStereoDisparity::block_match<E, D>(config);
        alg->process(left, right);

        const double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*
                                 config.max_per_pixel_error;
        const bool squared = error != DisparityError::SAD;
        Gray<D> disparity(30, 20);
        Gray<F32> score(30, 20);

        if (error == DisparityError::SAD || error == DisparityError::SSD) {
            using S = BlockScoreType<E>;
            auto select = make_shared<SelectErrorWithChecks<S, D>>(
                    max_error, config.validate_r_to_l, config.texture, squared);
            std::shared_ptr<BlockRowScore<E, S>> row_score;
            if (error == DisparityError::SAD)
                row_score = make_shared<BlockRowScoreSad<E>>();
            else
                row_score = make_shared<BlockRowScoreSsd<E>>();
            DisparityBlockMatchRowFormat<E, S> expected(config.region_radius_x, config.region_radius_y,
                                                        row_score, select);
            expected.configure(config.disparity_min, config.disparity_range);
            expected.set_border(config.border);
            expected.set_catastrophic_reset(config.catastrophic_reset);
            select->set_output(disparity, &score);
            expected.process(left, right);
        } else {
            Gray<F32> adjusted_left(1, 1), adjusted_right(1, 1);
            if (error == DisparityError::NCC && normalize_input) {
                NormalizeParameters parameters;
                ImageNormalization::zero_mean_max_one(left, adjusted_left, parameters);
                ImageNormalization::apply(right, parameters, adjusted_right);
            } else {
                convert(left, adjusted_left);
                convert(right, adjusted_right);
            }

            std::shared_ptr<BlockRowScore<F32, F32>> row_score;
            std::shared_ptr<DisparitySelect<F32>> select;
            if (error == DisparityError::NCC) {
                auto s = make_shared<SelectCorrelationWithChecks_F32<D>>(config.validate_r_to_l, config.texture);
                s->set_output(disparity, &score);
                select = s;
                row_score = make_shared<BlockRowScoreNcc>(config.region_radius_x, config.region_radius_y,
                                                          config.border, (F32)config.config_ncc.eps);
            } else {
                auto s = make_shared<SelectErrorWithChecks_F32<D>>(
                        max_error, config.validate_r_to_l, config.texture, true);
                s->set_output(disparity, &score);
                select = s;
                row_score = make_shared<BlockRowScoreZssd>(config.region_radius_x, config.region_radius_y,
                                                           config.border);
            }
            DisparityBlockMatchRowFormat<F32, F32> expected(config.region_radius_x, config.region_radius_y,
                                                            row_score, select);
            expected.configure(config.disparity_min, config.disparity_range);
            expected.set_border(config.border);
            expected.set_catastrophic_reset(config.catastrophic_reset);
            expected.process(adjusted_left, adjusted_right);

            auto& pre = dynamic_cast<const StereoDisparityBlockMatchPreFilter<E, D>&>(*alg);
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
    void compare_every_error_to_hand_built() {
        for (DisparityError error : {DisparityError::SAD, DisparityError::SSD, DisparityError::ZSSD,
                                     DisparityError::NCC}) {
            compare_error_to_hand_built<E, U8>(error, true, REFLECT);
            compare_error_to_hand_built<E, F32>(error, true, REFLECT);
            compare_error_to_hand_built<E, F32>(error, true, ZERO);
        }
        compare_error_to_hand_built<E, F32>(DisparityError::NCC, false, REFLECT);
    }
}

TEST(FactoryStereoDisparity, block_match_errors_same_as_hand_built_U8) {
    compare_every_error_to_hand_built<U8>();
}

TEST(FactoryStereoDisparity, block_match_errors_same_as_hand_built_F32) {
    compare_every_error_to_hand_built<F32>();
}

/// The local statistics of ZSSD and NCC need a region radius of at least 1, same as BoofCV
TEST(FactoryStereoDisparity, block_match_zssd_ncc_zero_radius) {
    Gray<U8> left(1, 1), right(1, 1);
    create_pair(left, right);
    for (DisparityError error : {DisparityError::ZSSD, DisparityError::NCC}) {
        ConfigDisparityBM config = create_sad_config(true);
        config.error_type = error;
        config.region_radius_x = 0;
        auto alg = FactoryStereoDisparity::block_match<U8, F32>(config);
        EXPECT_THROW(alg->process(left, right), invalid_argument);
    }
}

TEST(FactoryCensusTransform, variant) {
    EXPECT_NE(nullptr, (FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, REFLECT)));
    EXPECT_NE(nullptr, (FactoryCensusTransform::variant<U8, S32>(CensusVariants::BLOCK_5_5, REFLECT)));
    // census image type must match the variant
    EXPECT_THROW((FactoryCensusTransform::variant<U8, S32>(CensusVariants::BLOCK_3_3, REFLECT)), invalid_argument);
    EXPECT_THROW((FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_5_5, REFLECT)), invalid_argument);
    for (CensusVariants variant : {CensusVariants::BLOCK_7_7, CensusVariants::BLOCK_9_7, CensusVariants::BLOCK_13_5,
                                   CensusVariants::CIRCLE_9}) {
        EXPECT_NE(nullptr, (FactoryCensusTransform::variant<U8, S64>(variant, REFLECT)));
        EXPECT_THROW((FactoryCensusTransform::variant<U8, S32>(variant, REFLECT)), invalid_argument);
    }
    EXPECT_THROW((FactoryCensusTransform::variant<U8, S64>(CensusVariants::BLOCK_5_5, REFLECT)), invalid_argument);
    EXPECT_THROW((FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, NORMALIZED)), invalid_argument);
}

/// Each S64 variant samples the block or circle it's named after, one point per bit
TEST(FactoryCensusTransform, samples) {
    struct Expected { CensusVariants variant; int32_t radius_x, radius_y; };
    for (Expected e : {Expected{CensusVariants::BLOCK_7_7, 3, 3}, {CensusVariants::BLOCK_9_7, 4, 3},
                       {CensusVariants::BLOCK_13_5, 6, 2}, {CensusVariants::CIRCLE_9, 4, 4}}) {
        auto samples = FactoryCensusTransform::samples(e.variant);
        ASSERT_EQ(census_bits(e.variant), samples.size());
        int32_t max_x = 0, max_y = 0;
        for (const auto& p : samples) {
            max_x = std::max(max_x, std::abs(p.x));
            max_y = std::max(max_y, std::abs(p.y));
        }
        EXPECT_EQ(e.radius_x, max_x);
        EXPECT_EQ(e.radius_y, max_y);
    }
    EXPECT_THROW(FactoryCensusTransform::samples(CensusVariants::BLOCK_5_5), invalid_argument);
}

/// SKIP leaves the image border untouched, same as BoofCV
TEST(FactoryCensusTransform, variant_skip_border) {
    Gray<U8> input(12, 10);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);

    Gray<U8> expected(12, 10), found(12, 10);
    ImageMiscOps::fill(expected, (U8)7);
    ImageMiscOps::fill(found, (U8)7);
    CensusTransform::dense3x3(input, expected, (ImageBorder<U8>*)nullptr);
    FactoryCensusTransform::variant<U8, U8>(CensusVariants::BLOCK_3_3, SKIP)->process(input, found);
    check_equals(expected, found, (U8)0);
}

TEST(FactoryStereoDisparity, block_match_unsupported) {
    // subpixel and the disparity type must agree
    EXPECT_THROW((FactoryStereoDisparity::block_match<U8, U8>(create_sad_config(true))), invalid_argument);
    EXPECT_THROW((FactoryStereoDisparity::block_match<U8, F32>(create_sad_config(false))), invalid_argument);
    // only U8 and F32 images
    EXPECT_THROW((FactoryStereoDisparity::block_match<U16, F32>(create_sad_config(true))), invalid_argument);
    // invalid configuration
    ConfigDisparityBM config = create_sad_config(true);
    config.disparity_range = 0;
    EXPECT_THROW((FactoryStereoDisparity::block_match<U8, F32>(config)), invalid_argument);
}

TEST(StereoDisparityBlockMatch, getters) {
    auto alg = FactoryStereoDisparity::block_match<U8, F32>(create_sad_config(true));
    EXPECT_EQ(-2, alg->get_disparity_min());
    EXPECT_EQ(12u, alg->get_disparity_range());
    EXPECT_EQ(12, alg->get_invalid_value());
    EXPECT_EQ(0, alg->get_border_x());
    EXPECT_EQ(1, alg->get_border_y());
}

TEST(StereoDisparityBlockMatch, score_enabled) {
    Gray<U8> left(1, 1), right(1, 1);
    create_stereo_pair(left, right, 30, 20, 3);
    ConfigDisparityBM config = create_sad_config(false);
    config.save_score = false;
    auto alg = FactoryStereoDisparity::block_match<U8, U8>(config);
    alg->process(left, right);
    EXPECT_EQ(nullptr, alg->get_score());

    alg->set_score_enabled(true);
    alg->process(left, right);
    ASSERT_NE(nullptr, alg->get_score());
    EXPECT_EQ(30u, alg->get_score()->width);
    EXPECT_EQ(20u, alg->get_score()->height);
}

/// With every check off BoofCV's selector doesn't write the score, but it's still allocated
TEST(StereoDisparityBlockMatch, basic_select_score_not_written) {
    Gray<U8> left(1, 1), right(1, 1);
    create_stereo_pair(left, right, 30, 20, 3);
    ConfigDisparityBM config = create_sad_config(false);
    config.max_per_pixel_error = -1;
    config.validate_r_to_l = -1;
    config.texture = 0;
    auto alg = FactoryStereoDisparity::block_match<U8, U8>(config);
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

TEST(StereoDisparityBlockMatch, caller_owned_images_must_match_shape) {
    Gray<U8> left(1, 1), right(1, 1);
    create_stereo_pair(left, right, 30, 20, 3);
    auto alg = FactoryStereoDisparity::block_match<U8, U8>(create_sad_config(false));
    Gray<U8> disparity(29, 20);
    EXPECT_THROW(alg->process(left, right, disparity, nullptr), invalid_argument);
    Gray<U8> disparity_ok(30, 20);
    Gray<F32> score(30, 19);
    EXPECT_THROW(alg->process(left, right, disparity_ok, &score), invalid_argument);
}
