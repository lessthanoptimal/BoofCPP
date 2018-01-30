#include "gtest/gtest.h"
#include "binary_ops.h"

using namespace std;
using namespace boofcv;

// TODO Add subimage test for all functions
void set3x3( Gray<U8>& image, int* values ) {
    image.reshape(3,3);

    for( int i = 0; i < 9; i++ ) {
        image.data[i] = (U8)values[i];
    }
}

TEST(IndependentMethod, logicAnd) {
    Gray<U8> image0(5,6);
    Gray<U8> image1(5,6);
    Gray<U8> output;

    image0.at(0,0)=1;
    image0.at(1,1)=1;
    image0.at(2,2)=0;

    image1.at(0,0)=0;
    image1.at(1,1)=1;
    image1.at(2,2)=1;

    logicAnd(image0,image1,output);

    EXPECT_EQ(0,output.at(0,0));
    EXPECT_EQ(1,output.at(1,1));
    EXPECT_EQ(0,output.at(0,1));
    EXPECT_EQ(0,output.at(2,2));
}


TEST(IndependentMethod, logicOr) {
    Gray<U8> image0(5,6);
    Gray<U8> image1(5,6);
    Gray<U8> output;

    image0.at(0,0)=1;
    image0.at(1,1)=1;
    image0.at(2,2)=0;

    image1.at(0,0)=0;
    image1.at(1,1)=1;
    image1.at(2,2)=1;

    logicOr(image0,image1,output);

    EXPECT_EQ(1,output.at(0,0));
    EXPECT_EQ(1,output.at(1,1));
    EXPECT_EQ(0,output.at(0,1));
    EXPECT_EQ(1,output.at(2,2));
}


TEST(IndependentMethod, erode4) {
    Gray<U8> img,out;

    int case0[] = {0,0,0, 0,1,0, 0,0,0};
    int case1[] = {0,1,0, 1,1,1, 0,1,0};
    int case2[] = {0,0,0, 1,1,1, 0,1,0};
    int case3[] = {0,1,0, 0,1,1, 0,1,0};
    int case4[] = {0,1,0, 1,1,0, 0,1,0};
    int case5[] = {0,1,0, 1,1,1, 0,0,0};
    int case6[] = {1,1,1, 1,1,1, 1,1,1};

    set3x3(img,case0); erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case1); erode4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case2); erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case3); erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case4); erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case5); erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case6); erode4(img,out); EXPECT_EQ(1,out.at(1,1));
}


TEST(IndependentMethod, dilate4) {
    Gray<U8> img,out;

    int case0[] = {0,0,0, 0,1,0, 0,0,0};
    int case1[] = {0,1,0, 0,0,0, 0,0,0};
    int case2[] = {0,0,0, 1,0,0, 0,0,0};
    int case3[] = {0,0,0, 0,0,1, 0,0,0};
    int case4[] = {0,0,0, 0,0,0, 0,1,0};
    int case5[] = {1,0,0, 0,0,0, 0,0,0};
    int case6[] = {0,0,0, 0,0,0, 1,0,0};

    set3x3(img,case0); dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case1); dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case2); dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case3); dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case4); dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case5); dilate4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case6); dilate4(img,out); EXPECT_EQ(0,out.at(1,1));
}

TEST(IndependentMethod, ThresholdOps_threshold) {
    Gray<U8> img(4,5);
    Gray<U8> out(4,5);

    img.at(1,2) = 200;
    img.at(2,2) = 150;
    img.at(2,1) = 100;
    img.at(0,1) = 100;

    ThresholdOps::threshold(img,(U8)100,true,out);

    for( uint32_t y = 0; y < 5; y++ ) {
        for( uint32_t x = 0; x < 4; x++ ) {
            if( img.at(x,y) <= 100 ) {
                EXPECT_EQ(1,out.at(x,y));
            } else {
                EXPECT_EQ(0,out.at(x,y));
            }
        }
    }

    ThresholdOps::threshold(img,(U8)100,false,out);

    for( uint32_t y = 0; y < 5; y++ ) {
        for( uint32_t x = 0; x < 4; x++ ) {
            if( img.at(x,y) > 100 ) {
                EXPECT_EQ(1,out.at(x,y));
            } else {
                EXPECT_EQ(0,out.at(x,y));
            }
        }
    }
}