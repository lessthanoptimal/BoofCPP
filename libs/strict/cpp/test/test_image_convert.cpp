#include "gtest/gtest.h"
#include "boofcv/image_convert.h"
#include "testing_utils.h"

using namespace std;
using namespace boofcv;

class convert_averageTest : public boofcv::BoofCPPTest {};

TEST_F(convert_averageTest, interleaved_U8) {
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
class convertTest : public boofcv::BoofCPPTest {};

TEST_F(convertTest, U8_to_F32) {
    Gray<U8> input(9, 7);
    for (uint32_t i = 0; i < input.data_length; i++)
        input.data[i] = (U8)(i*37);
    for (bool sub : {false, true}) {
        Gray<U8> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, 9, 7);
        Gray<F32> output(1, 1);
        convert(in, output);
        ASSERT_EQ(9u, output.width);
        ASSERT_EQ(7u, output.height);
        for (uint32_t y = 0; y < 7; y++) {
            for (uint32_t x = 0; x < 9; x++)
                ASSERT_EQ((F32)input.at(x, y), output.at(x, y));
        }
        if (sub)
            in.subimage = false;
    }
}
