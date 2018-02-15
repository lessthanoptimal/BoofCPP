#include "gtest/gtest.h"
#include "base_types.h"
#include "image_misc_ops.h"

using namespace boofcv;
using namespace std;

TEST(ImageMiscOps, fill) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)8);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            ASSERT_EQ(8, image.at(x,y));
        }
    }

}

TEST(ImageMiscOps, fill_subimage) {
    Gray<U8> image(5,6);
    Gray<U8> subimage = image.makeSubimage(1,1,4,5);

    ImageMiscOps::fill(subimage,(U8)8);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            if( x >= 1 && x < image.width-1 && y >= 1 && y < image.height-1 ) {
                ASSERT_EQ(8, image.at(x, y));
            } else {
                ASSERT_EQ(0, image.at(x, y));
            }
        }
    }

}

TEST(ImageMiscOps, fill_border) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill_border(image,(U8)8,2);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            if( y < 2 || y >= 4 || x < 2 || x >= 3)
                ASSERT_EQ(8, image.at(x,y));
            else
                ASSERT_EQ(0, image.at(x,y));
        }
    }
}

TEST(ImageMiscOps, fill_border_LargeRadius) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill_border(image,(U8)8,6);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            ASSERT_EQ(8, image.at(x,y));
        }
    }
}

TEST(ImageMiscOps, fill_uniform_INTEGER) {
    Gray<U8> image(30,40);

    U8 min_value = 5;
    U8 max_value = 10;

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,min_value,max_value,gen);

    int N = max_value-min_value;
    int histogram[N];
    for( int i = 0; i < N; i++ ) {
        histogram[i] = 0;
    }
    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            histogram[image.at(x,y)-min_value]++;
            ASSERT_TRUE(image.at(x,y) >= min_value && image.at(x,y) < max_value);
        }
    }
    // crude test to see if it filled in
    for( int i = 0; i < N; i++ ) {
        ASSERT_TRUE(histogram[i]>30);
    }
}

TEST(ImageMiscOps, fill_uniform_FLOAT) {
    Gray<F32> image(30,40);

    F32 min_value = 5;
    F32 max_value = 10;

    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image,min_value,max_value,gen);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            ASSERT_TRUE(image.at(x,y) >= min_value && image.at(x,y) <= max_value);
        }
    }
}
