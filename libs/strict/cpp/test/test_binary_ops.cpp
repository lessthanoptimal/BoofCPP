#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/binary_ops.h"
#include "variant_strict.h"
#include "shared/threshold_checks.h"

using namespace std;
using namespace boofcv;

// TODO Add subimage test for all functions
void set3x3( Gray<U8>& image, int* values ) {
    image.reshape(3,3);

    for( int i = 0; i < 9; i++ ) {
        image.data[i] = (U8)values[i];
    }
}

class BinaryImageOpsTest : public boofcv::BoofCPPTest {};

TEST_F(BinaryImageOpsTest, logicAnd) {
    Gray<U8> image0(5,6);
    Gray<U8> image1(5,6);
    Gray<U8> output;

    image0.at(0,0)=1;
    image0.at(1,1)=1;
    image0.at(2,2)=0;

    image1.at(0,0)=0;
    image1.at(1,1)=1;
    image1.at(2,2)=1;

    BinaryImageOps::logicAnd(image0,image1,output);

    EXPECT_EQ(0,output.at(0,0));
    EXPECT_EQ(1,output.at(1,1));
    EXPECT_EQ(0,output.at(0,1));
    EXPECT_EQ(0,output.at(2,2));
}


TEST_F(BinaryImageOpsTest, logicOr) {
    Gray<U8> image0(5,6);
    Gray<U8> image1(5,6);
    Gray<U8> output;

    image0.at(0,0)=1;
    image0.at(1,1)=1;
    image0.at(2,2)=0;

    image1.at(0,0)=0;
    image1.at(1,1)=1;
    image1.at(2,2)=1;

    BinaryImageOps::logicOr(image0,image1,output);

    EXPECT_EQ(1,output.at(0,0));
    EXPECT_EQ(1,output.at(1,1));
    EXPECT_EQ(0,output.at(0,1));
    EXPECT_EQ(1,output.at(2,2));
}


TEST_F(BinaryImageOpsTest, erode4) {
    Gray<U8> img,out;

    int case0[] = {0,0,0, 0,1,0, 0,0,0};
    int case1[] = {0,1,0, 1,1,1, 0,1,0};
    int case2[] = {0,0,0, 1,1,1, 0,1,0};
    int case3[] = {0,1,0, 0,1,1, 0,1,0};
    int case4[] = {0,1,0, 1,1,0, 0,1,0};
    int case5[] = {0,1,0, 1,1,1, 0,0,0};
    int case6[] = {1,1,1, 1,1,1, 1,1,1};

    set3x3(img,case0); BinaryImageOps::erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case1); BinaryImageOps::erode4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case2); BinaryImageOps::erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case3); BinaryImageOps::erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case4); BinaryImageOps::erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case5); BinaryImageOps::erode4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case6); BinaryImageOps::erode4(img,out); EXPECT_EQ(1,out.at(1,1));
}


TEST_F(BinaryImageOpsTest, dilate4) {
    Gray<U8> img,out;

    int case0[] = {0,0,0, 0,1,0, 0,0,0};
    int case1[] = {0,1,0, 0,0,0, 0,0,0};
    int case2[] = {0,0,0, 1,0,0, 0,0,0};
    int case3[] = {0,0,0, 0,0,1, 0,0,0};
    int case4[] = {0,0,0, 0,0,0, 0,1,0};
    int case5[] = {1,0,0, 0,0,0, 0,0,0};
    int case6[] = {0,0,0, 0,0,0, 1,0,0};

    set3x3(img,case0); BinaryImageOps::dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case1); BinaryImageOps::dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case2); BinaryImageOps::dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case3); BinaryImageOps::dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case4); BinaryImageOps::dilate4(img,out); EXPECT_EQ(1,out.at(1,1));
    set3x3(img,case5); BinaryImageOps::dilate4(img,out); EXPECT_EQ(0,out.at(1,1));
    set3x3(img,case6); BinaryImageOps::dilate4(img,out); EXPECT_EQ(0,out.at(1,1));
}

class ThresholdOpsTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdOpsTest, threshold) {
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

/**
 * A threshold wider than the pixel type must not wrap around. Matches Java where an int threshold is used
 */
TEST_F(ThresholdOpsTest, threshold_outside_range) {
    Gray<U8> img(4,5);
    Gray<U8> out(4,5);
    img.at(1,2) = 255;
    img.at(2,2) = 150;

    ThresholdOps::threshold(img,(int32_t)300,true,out);
    for( uint32_t i = 0; i < out.total_pixels(); i++ ) EXPECT_EQ(1,out.data[i]);
    ThresholdOps::threshold(img,(int32_t)300,false,out);
    for( uint32_t i = 0; i < out.total_pixels(); i++ ) EXPECT_EQ(0,out.data[i]);
    ThresholdOps::threshold(img,(int32_t)-1,true,out);
    for( uint32_t i = 0; i < out.total_pixels(); i++ ) EXPECT_EQ(0,out.data[i]);
    ThresholdOps::threshold(img,(int32_t)-1,false,out);
    for( uint32_t i = 0; i < out.total_pixels(); i++ ) EXPECT_EQ(1,out.data[i]);
}

class GlobalOtsuBinaryFilterTest : public boofcv::BoofCPPTest {};

TEST_F(GlobalOtsuBinaryFilterTest, scale) { threshold_checks::global_otsu_scale(); }
TEST_F(GlobalOtsuBinaryFilterTest, min_value_offset) { threshold_checks::global_otsu_min_value_offset(); }

class GlobalFixedBinaryFilterTest : public boofcv::BoofCPPTest {};

TEST_F(GlobalFixedBinaryFilterTest, double_threshold) { threshold_checks::global_fixed_double_threshold(); }

class LocalMeanBinaryFilterTest : public boofcv::BoofCPPTest {};

TEST_F(LocalMeanBinaryFilterTest, naive) { threshold_checks::local_mean_naive(); }
TEST_F(LocalMeanBinaryFilterTest, zero_radius_U8) { threshold_checks::local_mean_zero_radius<U8>(); }
TEST_F(LocalMeanBinaryFilterTest, zero_radius_F32) { threshold_checks::local_mean_zero_radius<F32>(); }

TEST_F(ThresholdOpsTest, computeOtsu) {
    // This is intentionally omitted and is handled by comparing it to Java
    // a port of the java unit test would be complex
}

class ComputeOtsuTest : public boofcv::BoofCPPTest {};

TEST_F(ComputeOtsuTest, ComputeOtsu) {
    // Test of the class ComputeOtsu is intentionally omitted. Done by comparing results to Java
}