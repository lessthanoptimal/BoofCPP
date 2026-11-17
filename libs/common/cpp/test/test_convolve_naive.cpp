#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/convolve_naive.h"
#include "boofcv/image_misc_ops.h"

using namespace std;
using namespace boofcv;

U8 horizontal_norm(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const Gray<U8>& image )
{
    S32 total = 0;
    S32 weight = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x+i-kernel.offset,y)) {
            S32 w = kernel.at(i);
            U8 v = image.at(x+i-kernel.offset,y);

            total += w*v;
            weight += w;
        }
    }

    return static_cast<U8>((total + weight/2)/weight);
}

class ConvolveNormalizedNaiveTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveNormalizedNaiveTest, horizontal_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ConvolveNormalizedNaive::horizontal(kernel,input,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            S32 expected = horizontal_norm(x,y,kernel,input);
            S32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 horizontal_norm(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const Gray<F32>& image )
{
    F32 total = 0;
    F32 weight = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x+i-kernel.offset,y)) {
            F32 w = kernel.at(i);
            F32 v = image.at(x+i-kernel.offset,y);

            total += w*v;
            weight += w;
        }
    }

    return total/weight;
}

TEST_F(ConvolveNormalizedNaiveTest, horizontal_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ConvolveNormalizedNaive::horizontal(kernel,input,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = horizontal_norm(x,y,kernel,input);
            F32 found = output.at(x,y);
            ASSERT_FLOAT_EQ(expected,found);
        }
    }
}

U8 vertical_norm(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const Gray<U8>& image )
{
    S32 total = 0;
    S32 weight = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x,y+i-kernel.offset)) {
            S32 w = kernel.at(i);
            S32 v = image.at(x,y+i-kernel.offset);

            total += w*v;
            weight += w;
        }
    }

    return static_cast<U8>((total + weight/2)/weight);
}

TEST_F(ConvolveNormalizedNaiveTest, vertical_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50,gen);
    Gray<U8> output(15,16);

    ConvolveNormalizedNaive::vertical(kernel, input, output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = vertical_norm(x,y,kernel,input);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}


F32 vertical_norm(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const Gray<F32>& image )
{
    F32 total = 0;
    F32 weight = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x,y+i-kernel.offset)) {
            F32 w = kernel.at(i);
            F32 v = image.at(x,y+i-kernel.offset);

            total += w*v;
            weight += w;
        }
    }

    return total/weight;
}

TEST_F(ConvolveNormalizedNaiveTest, vertical_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50,gen);
    Gray<F32> output(15,16);

    ConvolveNormalizedNaive::vertical(kernel, input, output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = vertical_norm(x,y,kernel,input);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

U8 horizontal(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const ImageBorder<U8>& image )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t xx = (int32_t)(x+i)-(int32_t)kernel.offset;

        S32 w = kernel.at(i);
        U8 v = image.get(xx,y);

        total += w*v;
    }

    return (U8)total;
}

class ConvolveNaiveTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveNaiveTest, horizontal_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    ConvolveNaive::horizontal(kernel,border,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = horizontal(x,y,kernel,border);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

U8 vertical(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const ImageBorder<U8>& image )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t yy = (int32_t)(y+i)-(int32_t)kernel.offset;

        S32 w = kernel.at(i);
        U8 v = image.get(x,yy);

        total += w*v;
    }

    return (U8)total;
}

TEST_F(ConvolveNaiveTest, vertical_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    ConvolveNaive::vertical(kernel,border,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = vertical(x,y,kernel,border);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

U8 convolve2D(uint32_t x , uint32_t y , const Kernel2D<S32>& kernel , const ImageBorder<U8>& image )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        int32_t yy = (int32_t) (y + i) - (int32_t) kernel.offset;

        for (uint32_t j = 0; j < kernel.width; j++) {
            int32_t xx = (int32_t) (x + j) - (int32_t) kernel.offset;

            S32 w = kernel.at(j,i);
            U8 v = image.get(xx, yy);

            total += w * v;
        }
    }

    return (U8)total;
}

TEST_F(ConvolveNaiveTest, convolve_2D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel2D<S32> kernel(3,{{1,2,3,4},{5,6,7,8},{4,0,5,6},{4,-1,3,-9}});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    ConvolveNaive::convolve(kernel,border,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = convolve2D(x,y,kernel,border);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 convolve2D(uint32_t x , uint32_t y , const Kernel2D<F32>& kernel , const ImageBorder<F32>& image )
{
    F32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        int32_t yy = (int32_t) (y + i) - (int32_t) kernel.offset;

        for (uint32_t j = 0; j < kernel.width; j++) {
            int32_t xx = (int32_t) (x + j) - (int32_t) kernel.offset;

            F32 w = kernel.at(j,i);
            F32 v = image.get(xx, yy);

            total += w * v;
        }
    }

    return total;
}

TEST_F(ConvolveNaiveTest, convolve_2D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel2D<F32> kernel(3,{{1,2,3,4},{5,6,7,8},{4,0,5,6},{4,-1,3,-9}});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ImageBorderValue<F32> border(5);
    border.setImage(input);

    ConvolveNaive::convolve(kernel,border,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = convolve2D(x,y,kernel,border);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

U8 horizontal_divide(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const ImageBorder<U8>& image, S32 divisor )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t xx = (int32_t)(x+i)-(int32_t)kernel.offset;

        S32 w = kernel.at(i);
        U8 v = image.get(xx,y);

        total += w*v;
    }

    return static_cast<U8>((total+divisor/2)/divisor);
}

TEST_F(ConvolveNaiveTest, horizontal_divide_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    S32 divisor = 16;

    ConvolveNaive::horizontal(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = horizontal_divide(x,y,kernel,border,divisor);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 horizontal_divide(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const ImageBorder<F32>& image, F32 divisor )
{
    F32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t xx = (int32_t)(x+i)-(int32_t)kernel.offset;

        F32 w = kernel.at(i);
        F32 v = image.get(xx,y);

        total += w*v;
    }

    return total/divisor;
}

TEST_F(ConvolveNaiveTest, horizontal_divide_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ImageBorderValue<F32> border(5);
    border.setImage(input);

    F32 divisor = 14.5f;

    ConvolveNaive::horizontal(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = horizontal_divide(x,y,kernel,border,divisor);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

S32 convolve2D_divide(uint32_t x , uint32_t y , const Kernel2D<S32>& kernel , const ImageBorder<U8>& image, S32 divisor )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        int32_t yy = (int32_t) (y + i) - (int32_t) kernel.offset;
        for (uint32_t j = 0; j < kernel.width; j++) {
            // going overboard on typecasting to make sure it's signed
            int32_t xx = (int32_t) (x + j) - (int32_t) kernel.offset;

            S32 w = kernel.at(j, i);
            U8 v = image.get(xx, yy);

            total += w * v;
        }
    }

    return static_cast<U8>((total+divisor/2)/divisor);
}


TEST_F(ConvolveNaiveTest, convolve2D_divide_int) {
    std::mt19937 gen(0xBEEF);
    Kernel2D<S32> kernel(3,{{1,2,3,4},{5,6,7,8},{4,0,5,6},{4,-1,3,-9}});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    S32 divisor = 14;

    ConvolveNaive::convolve(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            S32 expected = convolve2D_divide(x,y,kernel,border,divisor);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 convolve2D_divide(uint32_t x , uint32_t y , const Kernel2D<F32>& kernel , const ImageBorder<F32>& image, F32 divisor )
{
    F32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        int32_t yy = (int32_t) (y + i) - (int32_t) kernel.offset;
        for (uint32_t j = 0; j < kernel.width; j++) {
            // going overboard on typecasting to make sure it's signed
            int32_t xx = (int32_t) (x + j) - (int32_t) kernel.offset;

            F32 w = kernel.at(j, i);
            F32 v = image.get(xx, yy);

            total += w * v;
        }
    }

    return total/divisor;
}

TEST_F(ConvolveNaiveTest, convolve2D_divide_float) {
    std::mt19937 gen(0xBEEF);
    Kernel2D<F32> kernel(3,{{1,2,3,4},{5,6,7,8},{4,0,5,6},{4,-1,3,-9}});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ImageBorderValue<F32> border(5);
    border.setImage(input);

    F32 divisor = 14.5f;

    ConvolveNaive::convolve(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = convolve2D_divide(x,y,kernel,border,divisor);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

U8 vertical_divide(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const ImageBorder<U8>& image, S32 divisor )
{
    S32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t yy = (int32_t)(y+i)-(int32_t)kernel.offset;

        S32 w = kernel.at(i);
        U8 v = image.get(x,yy);

        total += w*v;
    }

    return static_cast<U8>((total+divisor/2)/divisor);
}

TEST_F(ConvolveNaiveTest, vertical_divide_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ImageBorderValue<U8> border(5);
    border.setImage(input);

    S32 divisor = 16;

    ConvolveNaive::vertical(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            U8 expected = vertical_divide(x,y,kernel,border,divisor);
            U8 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 vertical_divide(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const ImageBorder<F32>& image, F32 divisor )
{
    F32 total = 0;

    for (uint32_t i = 0; i < kernel.width; i++) {
        // going overboard on typecasting to make sure it's signed
        int32_t yy = (int32_t)(y+i)-(int32_t)kernel.offset;

        F32 w = kernel.at(i);
        F32 v = image.get(x,yy);

        total += w*v;
    }

    return total/divisor;
}

TEST_F(ConvolveNaiveTest, vertical_divide_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ImageBorderValue<F32> border(5);
    border.setImage(input);

    F32 divisor = 16.5f;

    ConvolveNaive::vertical(kernel,border,output,divisor);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = vertical_divide(x,y,kernel,border,divisor);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}
