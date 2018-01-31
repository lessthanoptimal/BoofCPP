#include "gtest/gtest.h"
#include "base_types.h"
#include "image_misc_ops.h"

using namespace boofcv;
using namespace std;

TEST(IndependentMethod, fill) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)8);

    for( uint32_t y = 0; y < image.height; y++ ) {
        for( uint32_t x = 0; x < image.height; x++ ) {
            EXPECT_EQ(5, image.at(x,y));
        }
    }

}
