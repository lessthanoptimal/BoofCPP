#include "gtest/gtest.h"
#include "image_convert.h"

using namespace std;
using namespace boofcv;

TEST(convert_average, interleaved_U8) {
    Interleaved<U8> color(5,6,3);
    Gray<U8> gray(5,6);

    // simple case
    color.at(2,3,0) = 4;
    color.at(2,3,1) = 6;
    color.at(2,3,2) = 11;

    // this will overflow if done with the wrong data type
    color.at(2,4,0) = 200;
    color.at(2,4,1) = 200;
    color.at(2,4,2) = 200;

    convert_average(color,gray);

    EXPECT_EQ(21/3, gray.at(2,3));
    EXPECT_EQ(200, gray.at(2,4));
    EXPECT_EQ(0, gray.at(2,5));
}