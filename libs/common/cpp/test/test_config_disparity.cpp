#include "gtest/gtest.h"
#include "boofcv/config_disparity.h"
#include "testing_utils.h"

#include <set>
#include <stdexcept>
#include <string>

using namespace std;
using namespace boofcv;

class ConfigDisparityBMTest : public boofcv::BoofCPPTest {};

TEST_F(ConfigDisparityBMTest, defaults_same_as_java) {
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

TEST_F(ConfigDisparityBMTest, visit_fields_unique_names) {
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

TEST_F(ConfigDisparityBMTest, check_validity) {
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

class CensusVariantsTest : public boofcv::BoofCPPTest {};

TEST_F(CensusVariantsTest, census_bits_same_as_java) {
    EXPECT_EQ(8u, census_bits(CensusVariants::BLOCK_3_3));
    EXPECT_EQ(24u, census_bits(CensusVariants::BLOCK_5_5));
    EXPECT_EQ(48u, census_bits(CensusVariants::BLOCK_7_7));
    EXPECT_EQ(62u, census_bits(CensusVariants::BLOCK_9_7));
    EXPECT_EQ(64u, census_bits(CensusVariants::BLOCK_13_5));
    EXPECT_EQ(56u, census_bits(CensusVariants::CIRCLE_9));
}
