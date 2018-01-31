#include "gtest/gtest.h"
#include "base_types.h"
#include "image_misc_ops.h"

using namespace boofcv;
using namespace std;

TEST(IndependentMethod, fill) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)8);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.width; x++ ) {
            ASSERT_EQ(8, image.at(x,y));
        }
    }

}

TEST(IndependentMethod, fill_uniform_INTEGER) {
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

TEST(IndependentMethod, fill_uniform_FLOAT) {
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
