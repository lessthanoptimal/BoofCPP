#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/image_border.h"
#include "boofcv/image_misc_ops.h"


using namespace std;
using namespace boofcv;

class ImageBorderValueTest : public boofcv::BoofCPPTest {};

TEST_F(ImageBorderValueTest, check) {
    ImageBorderValue<U8> border( (U8)60 );

    Gray<U8> image(30,40);

    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,rng);
    border.setImage(image);


    // test points outside the image
    ASSERT_EQ(60,border.get(-1,5));
    ASSERT_EQ(60,border.get(5,-1));
    ASSERT_EQ(60,border.get(31,5));
    ASSERT_EQ(60,border.get(5,41));

    // inside and inside edge cases
    ASSERT_EQ(image.at(3,4),border.get(3,4));
    ASSERT_EQ(image.at(0,0),border.get(0,0));
    ASSERT_EQ(image.at(29,39),border.get(29,39));
}

class ImageBorderExtendTest : public boofcv::BoofCPPTest {};

TEST_F(ImageBorderExtendTest, check) {
    ImageBorderExtend<U8> border;

    Gray<U8> image(30,40);

    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,rng);
    border.setImage(image);


    // test points outside the image
    ASSERT_EQ(image.at(0,5),border.get(-1,5));
    ASSERT_EQ(image.at(5,0),border.get(5,-1));
    ASSERT_EQ(image.at(29,5),border.get(31,5));
    ASSERT_EQ(image.at(5,39),border.get(5,41));

    // inside and inside edge cases
    ASSERT_EQ(image.at(3,4),border.get(3,4));
    ASSERT_EQ(image.at(0,0),border.get(0,0));
    ASSERT_EQ(image.at(29,39),border.get(29,39));
}

class ImageBorderWrapTest : public boofcv::BoofCPPTest {};

TEST_F(ImageBorderWrapTest, check) {
    ImageBorderWrap<U8> border;

    Gray<U8> image(30,40);

    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,rng);
    border.setImage(image);


    // test points outside the image
    ASSERT_EQ(image.at(29,5),border.get(-1,5));
    ASSERT_EQ(image.at(5,39),border.get(5,-1));
    ASSERT_EQ(image.at(1,5),border.get(31,5));
    ASSERT_EQ(image.at(5,1),border.get(5,41));

    // inside and inside edge cases
    ASSERT_EQ(image.at(3,4),border.get(3,4));
    ASSERT_EQ(image.at(0,0),border.get(0,0));
    ASSERT_EQ(image.at(29,39),border.get(29,39));
}

class ImageBorderReflectTest : public boofcv::BoofCPPTest {};

TEST_F(ImageBorderReflectTest, check) {
    ImageBorderReflect<U8> border;

    Gray<U8> image(30,40);

    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,rng);
    border.setImage(image);


    // test points outside the image
    ASSERT_EQ(image.at(1,5),border.get(-1,5));
    ASSERT_EQ(image.at(5,1),border.get(5,-1));
    ASSERT_EQ(image.at(27,5),border.get(31,5));
    ASSERT_EQ(image.at(5,37),border.get(5,41));

    // inside and inside edge cases
    ASSERT_EQ(image.at(3,4),border.get(3,4));
    ASSERT_EQ(image.at(0,0),border.get(0,0));
    ASSERT_EQ(image.at(29,39),border.get(29,39));
}

/// Images smaller than the distance outside are reflected more than once. Same as BoofCV's
/// TestBorderIndex1D_Reflect.smallerThanOffset
TEST_F(ImageBorderReflectTest, smaller_than_offset) {
    ImageBorderReflect<U8> border;
    int32_t x;
    for (int32_t index : {-1, 1, -2, 2}) {
        x = index; border.adjust(x, 1); EXPECT_EQ(0, x) << index;
    }
    // image is 0 1 2. Index -3 to 5 is 1 2 1 0 1 2 1 0 1
    x = -3; border.adjust(x, 3); EXPECT_EQ(1, x);
    x = -2; border.adjust(x, 3); EXPECT_EQ(2, x);
    x = 4; border.adjust(x, 3); EXPECT_EQ(0, x);
    x = 5; border.adjust(x, 3); EXPECT_EQ(1, x);
}

/// Images smaller than the distance outside are wrapped more than once. Same as BoofCV's
/// TestBorderIndex1D_Wrap.smallerThanOffset
TEST_F(ImageBorderWrapTest, smaller_than_offset) {
    ImageBorderWrap<U8> border;
    int32_t x;
    for (int32_t index : {-1, 1, -2, 2}) {
        x = index; border.adjust(x, 1); EXPECT_EQ(0, x) << index;
    }
    x = -3; border.adjust(x, 2); EXPECT_EQ(1, x);
    x = -4; border.adjust(x, 2); EXPECT_EQ(0, x);
    x = 4; border.adjust(x, 2); EXPECT_EQ(0, x);
    x = 5; border.adjust(x, 2); EXPECT_EQ(1, x);
}
