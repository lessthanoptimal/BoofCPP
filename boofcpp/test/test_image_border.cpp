#include "gtest/gtest.h"
#include "image_border.h"
#include "image_misc_ops.h"

#include "print_structures.h"

using namespace std;
using namespace boofcv;

TEST(ImageBorderValue, check) {
    ImageBorderValue<U8> border( (U8)60 );

    Gray<U8> image(30,40);

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,gen);
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

TEST(ImageBorderExtend, check) {
    ImageBorderExtend<U8> border;

    Gray<U8> image(30,40);

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,gen);
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

TEST(ImageBorderWrap, check) {
    ImageBorderWrap<U8> border;

    Gray<U8> image(30,40);

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,gen);
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

TEST(ImageBorderReflect, check) {
    ImageBorderReflect<U8> border;

    Gray<U8> image(30,40);

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,(U8)1,(U8)50,gen);
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
