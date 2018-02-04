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

template<class E>
class CompareToNaive {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;

    std::mt19937 gen;

    uint32_t borderX0,borderX1;
    uint32_t borderY0,borderY1;

    CompareToNaive() : gen(0xBEEF) {

    }

    void setImageSize( uint32_t width , uint32_t height ) {

        input.reshape(width,height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)50,gen);
        found.reshape(width,height);
        expected.reshape(width,height);
    }

    void setKernel( uint32_t width , uint32_t offset ) {
        kernel.reshape(width,offset);
        KernelOps::fill_uniform(kernel,(signed_type)0,(signed_type)10,gen);
    }

    void horizontal() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveNormalized_JustBorder::horizontal(kernel,input,found);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults();
    }

    void vertical() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveNormalized_JustBorder::vertical(kernel,input,found);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults();
    }

    void checkResults() {
        for( int y = 0; y < input.height; y++ ) {
            for( int x = 0; x < input.width; x++ ) {
                if( x < borderX0 || y < borderY0 || x >= input.width - borderX1 || y >= input.height - borderY1 )
                {
                    ASSERT_EQ( expected.at(x,y) , found.at(x,y) );
                }
            }
        }
    }

};

TEST(ConvolveNormalized_JustBorder, horizontal_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal();
        compare.setKernel(5,2);
        compare.horizontal();
        compare.setKernel(7,3);
        compare.horizontal();
        compare.setKernel(7,1);
        compare.horizontal();
        compare.setKernel(7,4);
        compare.horizontal();
    }
}

TEST(ConvolveNormalized_JustBorder, horizontal_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal();
        compare.setKernel(5,2);
        compare.horizontal();
        compare.setKernel(7,3);
        compare.horizontal();
        compare.setKernel(7,1);
        compare.horizontal();
        compare.setKernel(7,4);
        compare.horizontal();
    }
}

TEST(ConvolveNormalized_JustBorder, vertical_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical();
        compare.setKernel(5,2);
        compare.vertical();
        compare.setKernel(7,3);
        compare.vertical();
        compare.setKernel(7,1);
        compare.vertical();
        compare.setKernel(7,4);
        compare.vertical();
    }
}

TEST(ConvolveNormalized_JustBorder, vertical_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical();
        compare.setKernel(5,2);
        compare.vertical();
        compare.setKernel(7,3);
        compare.vertical();
        compare.setKernel(7,1);
        compare.vertical();
        compare.setKernel(7,4);
        compare.vertical();
    }
}