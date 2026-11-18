#include "gtest/gtest.h"
#include <random>
#include "boofcv/threshold_block_filters.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"
#include "variant_strict.h"
#include "shared/threshold_checks.h"

using namespace std;
using namespace boofcv;

class DummyThresholdBlockCommon : public ThresholdBlockCommon<Gray<U8>,Interleaved<U8>> {
public:
    explicit DummyThresholdBlockCommon( uint32_t width )
            : ThresholdBlockCommon(ConfigLength::fixed(width), true) {}


    void computeBlockStatistics(uint32_t x0 , uint32_t y0 ,
                                uint32_t width , uint32_t height ,
                                uint32_t indexStats , const Gray<U8>& input) override {

    }

    void thresholdBlock(uint32_t blockX0 , uint32_t blockY0 , const Gray<U8>& input, Gray<U8>& output ) override {

    }
};

class ThresholdBlockCommonTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockCommonTest, selectBlockSize) {
    uint32_t width = 30;

    DummyThresholdBlockCommon alg(width);

    alg.selectBlockSize(300,330,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(329,301,width);
    ASSERT_EQ(32,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(301,329,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(32,alg.blockHeight);
}

class ThresholdBlockMeanTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMeanTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_mean<U8>()); }
TEST_F(ThresholdBlockMeanTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_mean<F32>()); }

class ThresholdBlockOtsuTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockOtsuTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_otsu<U8>()); }
TEST_F(ThresholdBlockOtsuTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_otsu<F32>()); }

class ThresholdBlockMinMaxTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMinMaxTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_min_max<U8>()); }
TEST_F(ThresholdBlockMinMaxTest, standard_tests_F32) {
    threshold_checks::block_all(threshold_checks::block_min_max<F32>());
}
