#include "gtest/gtest.h"
#include <random>
#include "threshold_block_filters.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

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

template<class T, class S>
class ThresholdBlockTest
{
public:
    ThresholdBlockCommon<T,S>* alg;
    std::mt19937 gen;

    ThresholdBlockTest() : gen(0xBEEF) {
        alg = nullptr;
    }

    virtual ~ThresholdBlockTest()
    {
        if( alg != nullptr )
            delete alg;
    }

    virtual void create_algorithm(uint32_t requestedBlockWidth, double scale , bool down ) = 0;

    void toggle_down() {
        uint32_t width = 100, height = 120;

        typedef typename T::pixel_type E;
        T input(width,height);
        ImageMiscOps::fill_uniform(input,(E)0,(E)255,gen);

        Gray<U8> down(width,height);
        Gray<U8> up(width,height);

        // turn off texture so that the output's can be the inverse of each other
        create_algorithm(6,1.0,true);
        alg->process(input,down);
        create_algorithm(6,1.0,false);
        alg->process(input,up);

        for (uint32_t y = 0; y < down.height; y++) {
            for (uint32_t x = 0; x < down.width; x++) {
                ASSERT_TRUE(down.at(x,y) == !up.at(x,y));
            }
        }
    }

    void widthLargerThanImage() {
        T input(10,12);
        Gray<U8> output(input.width,input.height);

        create_algorithm(20,1.0,true);
        try {
            alg->process(input, output);
            ASSERT_TRUE(false);
        } catch( ... ) {

        }
    }

    void subimage() {
        typedef typename T::pixel_type E;

        T input(100,120);
        ImageMiscOps::fill_uniform(input,(E)0,(E)255,gen);

        Gray<U8> expected(input.width,input.height);

        T sub_input = create_subimage(input);
        Gray<U8> sub_output = create_subimage(expected);

        create_algorithm(14,1.0,true);

        alg->process(input,expected);
        alg->process(sub_input,sub_output);

        check_equals(expected,sub_output);

        // do this so that it frees the memory and valgrind doesn't complain
        sub_input.subimage = false;
        sub_output.subimage = false;
    }
};

template<class E>
class ThresholdBlockMeanTest : public ThresholdBlockTest<Gray<E>,Interleaved<E>>
{
public:
    bool thresholdFromLocalBlocks=true;

    void create_algorithm(uint32_t requestedBlockWidth, double scale , bool down ) override {
        if( this->alg != nullptr )
            delete  this->alg;
        ConfigLength c = ConfigLength::fixed(requestedBlockWidth);
        this->alg = new ThresholdBlockMean<E>(c,thresholdFromLocalBlocks,scale,down);
    }
};

TEST(ThresholdBlockMean, standard_tests_U8) {
    ThresholdBlockMeanTest<U8> standard_tests;

    standard_tests.toggle_down();
    standard_tests.widthLargerThanImage();
    standard_tests.subimage();
}

template<class E>
class ThresholdBlockOtsuTest : public ThresholdBlockTest<Gray<E>,Interleaved<S32>>
{
public:
    bool thresholdFromLocalBlocks=true;

    void create_algorithm(uint32_t requestedBlockWidth, double scale , bool down ) override {
        if( this->alg != nullptr )
            delete  this->alg;
        ConfigLength c = ConfigLength::fixed(requestedBlockWidth);
        this->alg = new ThresholdBlockOtsu<E>(false,c,0.0,scale,down,thresholdFromLocalBlocks);
    }
};

TEST(ThresholdBlockOtsu, standard_tests_U8) {
    ThresholdBlockOtsuTest<U8> standard_tests;

    standard_tests.toggle_down();
    standard_tests.widthLargerThanImage();
    standard_tests.subimage();
}

template<class E>
class ThresholdBlocMinMaxTest : public ThresholdBlockTest<Gray<E>,Interleaved<E>>
{
public:
    float minimumSpread = 1;
    bool thresholdFromLocalBlocks=true;

    void create_algorithm(uint32_t requestedBlockWidth, double scale , bool down ) override {
        if( this->alg != nullptr )
            delete  this->alg;
        ConfigLength c = ConfigLength::fixed(requestedBlockWidth);
        this->alg = new ThresholdBlockMinMax<E>(minimumSpread,c,thresholdFromLocalBlocks,(float)scale,down);
    }
};

TEST(ThresholdBlocMinMaxTest, standard_tests_U8) {
    ThresholdBlocMinMaxTest<U8> standard_tests;

    standard_tests.toggle_down();
    standard_tests.widthLargerThanImage();
    standard_tests.subimage();
}