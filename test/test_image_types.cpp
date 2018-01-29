#include "gtest/gtest.h"
#include "binary_ops.h"

using namespace std;
using namespace boofcv;

TEST(IndependentMethod, Gray_Constructor_WH) {
    Gray<U8> imgA(10, 20);

    EXPECT_EQ(10,imgA.width);
    EXPECT_EQ(20,imgA.height);
    EXPECT_EQ(10,imgA.stride);
    EXPECT_EQ(0,imgA.offset);
    EXPECT_EQ(false,imgA.subimage);
    EXPECT_EQ(200,imgA.data_length);
    for( uint32_t i = 0; i < 200; i++ ) {
        EXPECT_EQ(0,imgA.data[i]);
    }
}

TEST(IndependentMethod, Gray_Constructor_NoArguments) {
    Gray<U8> imgA;

    EXPECT_EQ(0,imgA.width);
    EXPECT_EQ(0,imgA.height);
    EXPECT_EQ(0,imgA.stride);
    EXPECT_EQ(0,imgA.offset);
    EXPECT_EQ(false,imgA.subimage);
    EXPECT_EQ(0,imgA.data_length);
}

// TODO Test at with a sub-image
TEST(IndependentMethod, Gray_at) {
    Gray<U8> imgA(10, 20);

    EXPECT_EQ(0,imgA.at(2,1));
    imgA.at(2,1) = 5;
    EXPECT_EQ(5,imgA.at(2,1));

    for( uint32_t i = 0; i < 200; i++ ) {
        if( i != 12 )
            EXPECT_EQ(0,imgA.data[i]);
        else
            EXPECT_EQ(5,imgA.data[i]);
    }
}

TEST(IndependentMethod, Gray_reshape) {
    Gray<U8> imgA(10, 20);

    U8 *orig_pts = imgA.data;

    // shrink it to a smaller size first
    imgA.reshape(10,8);
    EXPECT_EQ(10,imgA.width);
    EXPECT_EQ(8,imgA.height);
    EXPECT_EQ(10,imgA.stride);
    EXPECT_EQ(0,imgA.offset);
    EXPECT_EQ(false,imgA.subimage);
    EXPECT_EQ(200,imgA.data_length);
    EXPECT_EQ(orig_pts,imgA.data);
    for( uint32_t i = 0; i < 10*8; i++ ) {
        EXPECT_EQ(0,imgA.data[i]);
    }

    // it now requires a larger array
    imgA.reshape(12,20);
    EXPECT_EQ(12,imgA.width);
    EXPECT_EQ(20,imgA.height);
    EXPECT_EQ(12,imgA.stride);
    EXPECT_EQ(0,imgA.offset);
    EXPECT_EQ(false,imgA.subimage);
    EXPECT_EQ(12*20,imgA.data_length);
    EXPECT_NE(orig_pts,imgA.data);
    for( uint32_t i = 0; i < 12*20; i++ ) {
        EXPECT_EQ(0,imgA.data[i]);
    }
}