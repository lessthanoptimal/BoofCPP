#include "gtest/gtest.h"
#include "binary_ops.h"

using namespace std;
using namespace boofcv;

TEST(Gray, Constructor_WH) {
    Gray<U8> imgA(10, 20);

    ASSERT_EQ(10,imgA.width);
    ASSERT_EQ(20,imgA.height);
    ASSERT_EQ(10,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(200,imgA.data_length);
    for( uint32_t i = 0; i < 200; i++ ) {
        ASSERT_EQ(0,imgA.data[i]);
    }
}

TEST(Gray, Constructor_NoArguments) {
    Gray<U8> imgA;

    ASSERT_EQ(0,imgA.width);
    ASSERT_EQ(0,imgA.height);
    ASSERT_EQ(0,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(0,imgA.data_length);
}

// at in a subimage is tested in the subimage test
TEST(Gray, at) {
    Gray<U8> imgA(10, 20);

    ASSERT_EQ(0,imgA.at(2,1));
    imgA.at(2,1) = 5;
    ASSERT_EQ(5,imgA.at(2,1));

    for( uint32_t i = 0; i < 200; i++ ) {
        if( i != 12 )
            ASSERT_EQ(0,imgA.data[i]);
        else
            ASSERT_EQ(5,imgA.data[i]);
    }
}

TEST(Gray, reshape) {
    Gray<U8> imgA(10, 20);

    U8 *orig_pts = imgA.data;

    // shrink it to a smaller size first
    imgA.reshape(10,8);
    ASSERT_EQ(10,imgA.width);
    ASSERT_EQ(8,imgA.height);
    ASSERT_EQ(10,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(200,imgA.data_length);
    ASSERT_EQ(orig_pts,imgA.data);
    for( uint32_t i = 0; i < 10*8; i++ ) {
        ASSERT_EQ(0,imgA.data[i]);
    }

    // it now requires a larger array
    imgA.reshape(12,20);
    ASSERT_EQ(12,imgA.width);
    ASSERT_EQ(20,imgA.height);
    ASSERT_EQ(12,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(12*20,imgA.data_length);
    EXPECT_NE(orig_pts,imgA.data);
    for( uint32_t i = 0; i < 12*20; i++ ) {
        ASSERT_EQ(0,imgA.data[i]);
    }
}

TEST(Gray, makeSubimage) {
    Gray<U8> img(10, 20);
    Gray<U8> sub = img.makeSubimage(2,3,6,10);

    ASSERT_EQ(4,sub.width);
    ASSERT_EQ(7,sub.height);
    ASSERT_EQ(10,sub.stride);
    ASSERT_EQ(3*10+2,sub.offset);

    sub.at(0,0) = 2;
    ASSERT_EQ(2,img.at(2,3));
}

TEST(Planar, Constructor_WH) {

}

TEST(Planar, Constructor_NoArguments) {

}

TEST(Planar, at) {

}

TEST(Planar, reshape) {

}

TEST(Planar, setNumberOfBands) {

}

TEST(Planar, setTo) {

}

TEST(Planar, makeSubimage) {

}

TEST(Interleaved, Constructor_WH) {
    Interleaved<U8> imgA(10, 15,2);

    ASSERT_EQ(10,imgA.width);
    ASSERT_EQ(15,imgA.height);
    ASSERT_EQ(2,imgA.num_bands);
    ASSERT_EQ(20,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(300,imgA.data_length);
    for( uint32_t i = 0; i < imgA.data_length; i++ ) {
        ASSERT_EQ(0,imgA.data[i]);
    }
}

TEST(Interleaved, Constructor_NoArguments) {
    Interleaved<U8> img;

    ASSERT_EQ(0,img.width);
    ASSERT_EQ(0,img.height);
    ASSERT_EQ(0,img.num_bands);
    ASSERT_EQ(0,img.stride);
    ASSERT_EQ(0,img.offset);
    ASSERT_FALSE(img.subimage);
    ASSERT_EQ(0,img.data_length);
}

TEST(Interleaved, at) {
    Interleaved<U8> img(10, 15,2);

    ASSERT_EQ(0,img.at(2,1,0));
    img.at(2,1,0) = 5;
    ASSERT_EQ(5,img.at(2,1,0));

    for( uint32_t i = 0; i < 300; i++ ) {
        if( i != (1*10+2)*2+0 )
            ASSERT_EQ(0,img.data[i]);
        else
            ASSERT_EQ(5,img.data[i]);
    }
}

TEST(Interleaved, reshape) {
    Interleaved<U8> img(10, 15,2);

    U8 *orig_pts = img.data;

    // shrink it to a smaller size first
    img.reshape(10,8);
    ASSERT_EQ(10,img.width);
    ASSERT_EQ(8,img.height);
    ASSERT_EQ(2,img.num_bands);
    ASSERT_EQ(20,img.stride);
    ASSERT_EQ(0,img.offset);
    ASSERT_FALSE(img.subimage);
    ASSERT_EQ(300,img.data_length);
    ASSERT_EQ(orig_pts,img.data);
    for( uint32_t i = 0; i < 10*8*2; i++ ) {
        ASSERT_EQ(0,img.data[i]);
    }

    // it now requires a larger array
    img.reshape(12,20);
    ASSERT_EQ(12,img.width);
    ASSERT_EQ(20,img.height);
    ASSERT_EQ(2,img.num_bands);
    ASSERT_EQ(24,img.stride);
    ASSERT_EQ(0,img.offset);
    ASSERT_FALSE(img.subimage);
    ASSERT_EQ(12*20*2,img.data_length);
//    EXPECT_NE(orig_pts,imgA.data); // it's possible for it to be allocated the same memorty twice
    for( uint32_t i = 0; i < 12*20*2; i++ ) {
        ASSERT_EQ(0,img.data[i]);
    }
}

TEST(Interleaved, setNumberOfBands) {
    Interleaved<U8> img(10, 15, 2);

    img.setNumberOfBands(1);
    ASSERT_EQ(10,img.width);
    ASSERT_EQ(15,img.height);
    ASSERT_EQ(1,img.num_bands);
    ASSERT_EQ(10,img.stride);
    ASSERT_EQ(0,img.offset);
    ASSERT_FALSE(img.subimage);
    ASSERT_EQ(300,img.data_length);
    for( uint32_t i = 0; i < 150; i++ ) {
        ASSERT_EQ(0,img.data[i]);
    }

    img.setNumberOfBands(3);
    ASSERT_EQ(10,img.width);
    ASSERT_EQ(15,img.height);
    ASSERT_EQ(3,img.num_bands);
    ASSERT_EQ(30,img.stride);
    ASSERT_EQ(0,img.offset);
    ASSERT_FALSE(img.subimage);
    ASSERT_EQ(150*3,img.data_length);
    for( uint32_t i = 0; i < 150*3; i++ ) {
        ASSERT_EQ(0,img.data[i]);
    }
}

TEST(Interleaved, setTo) {
    Interleaved<U8> imgA(10, 15, 2);
    Interleaved<U8> imgB(4, 5, 1);

    imgB.at(1,2,0) = 10;

    imgA.setTo(imgB);
    ASSERT_EQ(4,imgA.width);
    ASSERT_EQ(5,imgA.height);
    ASSERT_EQ(1,imgA.num_bands);
    ASSERT_EQ(4,imgA.stride);
    ASSERT_EQ(0,imgA.offset);
    ASSERT_FALSE(imgA.subimage);
    ASSERT_EQ(300,imgA.data_length);
    for( uint32_t i = 0; i < 20; i++ ) {
        if( i == 2*4+1 ) {
            ASSERT_EQ(10, imgA.data[i]);
        } else {
            ASSERT_EQ(0, imgA.data[i]);
        }
    }
}

TEST(Interleaved, makeSubimage) {
    Interleaved<U8> img(10, 15, 2);

    Interleaved<U8> subimage = img.makeSubimage(2,3,8,5);

    ASSERT_EQ(6,subimage.width);
    ASSERT_EQ(2,subimage.height);
    ASSERT_EQ(2,subimage.num_bands);
    ASSERT_EQ(20,subimage.stride);
    ASSERT_EQ((3*10+2)*2,subimage.offset);
    ASSERT_EQ(true,subimage.subimage);
//    ASSERT_EQ(300,imgA.data_length); value of length doesn't matter. not used

    subimage.at(0,0,1) = 5;
    ASSERT_EQ(5,img.at(2,3,1));
}