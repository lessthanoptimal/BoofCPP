#include "gtest/gtest.h"
#include "base_types.h"
#include "image_statistics.h"
#include "image_misc_ops.h"

using namespace boofcv;
using namespace std;

TEST(IndependentMethod, min) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 6;

    ASSERT_EQ(6,ImageStatistics::min(image));
}

TEST(IndependentMethod, max) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 12;

    ASSERT_EQ(12,ImageStatistics::max(image));
}

TEST(IndependentMethod, sum) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);

    ASSERT_EQ(2*5*6,ImageStatistics::sum(image));
}

TEST(IndependentMethod, mean) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);

    ASSERT_EQ(2,ImageStatistics::mean(image));
}

TEST(IndependentMethod, histogramINTEGER) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 12;
    image.at(2,4) = 12;
    image.at(2,5) = 12;
    image.at(0,0) = 6;

    GrowArray<uint32_t> histogram(256);

    ImageStatistics::histogram(image,(U8)0,histogram);

    for( int i = 0; i < 256; i++ ) {
        if( i == 6 ) {
            ASSERT_EQ(1,histogram[i]);
        } else if( i == 12 ) {
            ASSERT_EQ(3,histogram[i]);
        } else if( i == 10 ) {
            ASSERT_EQ(5*6-4,histogram[i]);
        } else {
            ASSERT_EQ(0,histogram[i]);
        }
    }
}

TEST(IndependentMethod, histogramFLOAT) {
    Gray<F32> image(5,6);

    ImageMiscOps::fill(image,(F32)10);
    image.at(2,3) = 12.1f;
    image.at(2,4) = 12.5f;
    image.at(2,5) = 12.9f;
    image.at(0,0) = 6.4f;

    GrowArray<uint32_t> histogram(256);

    ImageStatistics::histogram(image,(F32)0,histogram);

    for( int i = 0; i < 256; i++ ) {
        if( i == 6 ) {
            ASSERT_EQ(1,histogram[i]);
        } else if( i == 12 ) {
            ASSERT_EQ(3,histogram[i]);
        } else if( i == 10 ) {
            ASSERT_EQ(5*6-4,histogram[i]);
        } else {
            ASSERT_EQ(0,histogram[i]);
        }
    }
}