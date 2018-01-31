#include "gtest/gtest.h"
#include "threshold_block_filters.h"

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

TEST(ThresholdBlockCommon, selectBlockSize) {
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

TEST(ThresholdBlockMean, ThresholdBlockMean) {
    // TODO implement a generic test for block algorithms a is done in java
}