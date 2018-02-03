#include "gtest/gtest.h"
#include "convolve.h"
#include "image_misc_ops.h"

using namespace std;
using namespace boofcv;


S32 horizontal(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const Gray<U8>& image )
{
    S32 total = 0;
    S32 weight = 0;

    for (int i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x+i-kernel.offset,y)) {
            S32 w = kernel.at(i);
            U8 v = image.at(x+i-kernel.offset,y);

            total += w*v;
            weight += w;
        }
    }

    return (total + weight/2)/weight;
}

TEST(ConvolveNormalizedNaive, horizontal_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, gen);
    Gray<U8> output(15,16);

    ConvolveNormalizedNaive::horizontal(kernel,input,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            S32 expected = horizontal(x,y,kernel,input);
            S32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}

F32 horizontal(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const Gray<F32>& image )
{
    F32 total = 0;
    F32 weight = 0;

    for (int i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x+i-kernel.offset,y)) {
            F32 w = kernel.at(i);
            F32 v = image.at(x+i-kernel.offset,y);

            total += w*v;
            weight += w;
        }
    }

    return total/weight;
}

TEST(ConvolveNormalizedNaive, horizontal_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50, gen);
    Gray<F32> output(15,16);

    ConvolveNormalizedNaive::horizontal(kernel,input,output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = horizontal(x,y,kernel,input);
            F32 found = output.at(x,y);
            ASSERT_FLOAT_EQ(expected,found);
        }
    }
}

S32 vertical(uint32_t x , uint32_t y , const Kernel1D<S32>& kernel , const Gray<U8>& image )
{
    S32 total = 0;
    S32 weight = 0;

    for (int i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x,y+i-kernel.offset)) {
            S32 w = kernel.at(i);
            S32 v = image.at(x,y+i-kernel.offset);

            total += w*v;
            weight += w;
        }
    }

    return (total + weight/2)/weight;
}

TEST(ConvolveNormalizedNaive, vertical_1D_int) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<S32> kernel(4,{1,2,3,4,5,6});

    Gray<U8> input(15,16);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50,gen);
    Gray<U8> output(15,16);

    ConvolveNormalizedNaive::vertical(kernel, input, output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            S32 expected = vertical(x,y,kernel,input);
            S32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}


F32 vertical(uint32_t x , uint32_t y , const Kernel1D<F32>& kernel , const Gray<F32>& image )
{
    F32 total = 0;
    F32 weight = 0;

    for (int i = 0; i < kernel.width; i++) {
        if( image.isInBounds(x,y+i-kernel.offset)) {
            F32 w = kernel.at(i);
            F32 v = image.at(x,y+i-kernel.offset);

            total += w*v;
            weight += w;
        }
    }

    return total/weight;
}

TEST(ConvolveNormalizedNaive, vertical_1D_float) {
    std::mt19937 gen(0xBEEF);
    Kernel1D<F32> kernel(4,{1,2,3,4,5,6});

    Gray<F32> input(15,16);
    ImageMiscOps::fill_uniform(input, (F32)0, (F32)50,gen);
    Gray<F32> output(15,16);

    ConvolveNormalizedNaive::vertical(kernel, input, output);

    for (uint32_t y = 0; y < output.height; y++) {
        for (uint32_t x = 0; x < output.width; x++) {
            F32 expected = vertical(x,y,kernel,input);
            F32 found = output.at(x,y);
            ASSERT_EQ(expected,found);
        }
    }
}
