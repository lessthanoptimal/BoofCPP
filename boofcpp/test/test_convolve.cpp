#include <print_structures.h>
#include "gtest/gtest.h"
#include "convolve.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

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

TEST(ConvolveNormalizedNaive, horizontal_1D_int) {
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

TEST(ConvolveNormalizedNaive, horizontal_1D_float) {
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

TEST(ConvolveNormalizedNaive, vertical_1D_int) {
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

TEST(ConvolveNormalizedNaive, vertical_1D_float) {
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

TEST(ConvolveNaive, horizontal_1D_int) {
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

TEST(ConvolveNaive, vertical_1D_int) {
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

TEST(ConvolveNaive, convolve_2D_int) {
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

TEST(ConvolveNaive, convolve_2D_float) {
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

TEST(ConvolveNaive, horizontal_divide_1D_int) {
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

TEST(ConvolveNaive, horizontal_divide_1D_float) {
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


TEST(ConvolveNaive, convolve2D_divide_int) {
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

TEST(ConvolveNaive, convolve2D_divide_float) {
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

TEST(ConvolveNaive, vertical_divide_1D_int) {
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

TEST(ConvolveNaive, vertical_divide_1D_float) {
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

template<class E>
class CompareToNaive {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;
    Kernel2D<signed_type> kernel2;

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

    void setKernel( uint32_t width , uint32_t offset , bool has_negative=false) {
        kernel.reshape(width,offset);
        if( has_negative )
            KernelOps::fill_uniform(kernel,(signed_type)-10,(signed_type)10,gen);
        else
            KernelOps::fill_uniform(kernel,(signed_type)0,(signed_type)10,gen);
    }

    void setKernel2( uint32_t width , uint32_t offset , bool has_negative=false) {
        kernel2.reshape(width,offset);
        if( has_negative )
            KernelOps::fill_uniform(kernel2,(signed_type)-10,(signed_type)10,gen);
        else
            KernelOps::fill_uniform(kernel2,(signed_type)0,(signed_type)10,gen);
    }

    void horizontal_norm_border() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveNormalized_JustBorder::horizontal(kernel,input,found);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_border();
    }

    void vertical_norm_border() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveNormalized_JustBorder::vertical(kernel,input,found);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_border();
    }

    void horizontal_div_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        signed_type kernel_sum = kernel.sum();
        ConvolveNaive::horizontal(kernel,border,expected,kernel_sum);
        ConvolveImage_Inner::horizontal(kernel,input,found,kernel_sum);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void vertical_div_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        signed_type kernel_sum = kernel.sum();
        ConvolveNaive::vertical(kernel,border,expected,kernel_sum);
        ConvolveImage_Inner::vertical(kernel,input,found,kernel_sum);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void convolve_div_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        signed_type kernel_sum = kernel2.sum();

        ConvolveNaive::convolve(kernel2,border,expected,kernel_sum);
        ConvolveImage_Inner::convolve(kernel2,input,found,kernel_sum);

        borderX0 = kernel2.offset;
        borderX1 = kernel2.width-1-kernel2.offset;
        borderY0 = kernel2.offset;
        borderY1 = kernel2.width-1-kernel2.offset;

        checkResults_inner();
    }

    void horizontal_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::horizontal(kernel,border,expected);
        ConvolveImage_Inner::horizontal(kernel,input,found);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void horizontal_border() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::horizontal(kernel,border,expected);
        ConvolveImage_Border::horizontal(kernel,border,found);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_border();
    }

    void vertical_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::vertical(kernel,border,expected);
        ConvolveImage_Inner::vertical(kernel,input,found);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void vertical_border() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::vertical(kernel,border,expected);
        ConvolveImage_Border::vertical(kernel,border,found);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_border();
    }

    void convolve_inner() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::convolve(kernel2,border,expected);
        ConvolveImage_Inner::convolve(kernel2,input,found);

        borderX0 = kernel2.offset;
        borderX1 = kernel2.width-1-kernel2.offset;
        borderY0 = kernel2.offset;
        borderY1 = kernel2.width-1-kernel2.offset;

        checkResults_inner();
    }

    void convolve_border() {
        ImageBorderValue<E> border(0);
        border.setImage(input);

        ConvolveNaive::convolve(kernel2,border,expected);
        ConvolveImage_Border::convolve(kernel2,border,found);

        borderX0 = kernel2.offset;
        borderX1 = kernel2.width-1-kernel2.offset;
        borderY0 = kernel2.offset;
        borderY1 = kernel2.width-1-kernel2.offset;

        checkResults_border();
    }

    void normalized_horizontal() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveNormalized::horizontal(kernel,input,found);

        check_equals(expected,found);
    }

    void normalized_vertical() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveNormalized::vertical(kernel,input,found);

        check_equals(expected,found);
    }

    void normalized_convolve_1D() {
        Gray<E> workspace(input.width,input.height);
        ConvolveNormalizedNaive::horizontal(kernel,input,workspace);
        ConvolveNormalizedNaive::vertical(kernel,workspace,expected);
        ConvolveNormalized::convolve(kernel,input,found,workspace);

        check_equals(expected,found);
    }

    void checkResults_border() {
        check_equals_border(expected,found,borderX0,borderX1,borderY0,borderY1);
    }

    void checkResults_inner() {
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1);
    }

};

TEST(ConvolveNormalized_JustBorder, horizontal_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_norm_border();
        compare.setKernel(5,2);
        compare.horizontal_norm_border();
        compare.setKernel(7,3);
        compare.horizontal_norm_border();
        compare.setKernel(7,1);
        compare.horizontal_norm_border();
        compare.setKernel(7,4);
        compare.horizontal_norm_border();
    }
}

TEST(ConvolveNormalized_JustBorder, horizontal_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_norm_border();
        compare.setKernel(5,2);
        compare.horizontal_norm_border();
        compare.setKernel(7,3);
        compare.horizontal_norm_border();
        compare.setKernel(7,1);
        compare.horizontal_norm_border();
        compare.setKernel(7,4);
        compare.horizontal_norm_border();
    }
}

TEST(ConvolveNormalized_JustBorder, vertical_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_norm_border();
        compare.setKernel(5,2);
        compare.vertical_norm_border();
        compare.setKernel(7,3);
        compare.vertical_norm_border();
        compare.setKernel(7,1);
        compare.vertical_norm_border();
        compare.setKernel(7,4);
        compare.vertical_norm_border();
    }
}

TEST(ConvolveNormalized_JustBorder, vertical_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_norm_border();
        compare.setKernel(5,2);
        compare.vertical_norm_border();
        compare.setKernel(7,3);
        compare.vertical_norm_border();
        compare.setKernel(7,1);
        compare.vertical_norm_border();
        compare.setKernel(7,4);
        compare.vertical_norm_border();
    }
}

TEST(ConvolveImage_Inner, horizontal_div_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_div_inner();
        compare.setKernel(5,2);
        compare.horizontal_div_inner();
        compare.setKernel(7,3);
        compare.horizontal_div_inner();
        compare.setKernel(7,1);
        compare.horizontal_div_inner();
        compare.setKernel(7,4);
        compare.horizontal_div_inner();
    }
}

TEST(ConvolveImage_Inner, horizontal_div_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_div_inner();
        compare.setKernel(5,2);
        compare.horizontal_div_inner();
        compare.setKernel(7,3);
        compare.horizontal_div_inner();
        compare.setKernel(7,1);
        compare.horizontal_div_inner();
        compare.setKernel(7,4);
        compare.horizontal_div_inner();
    }
}

TEST(ConvolveImage_Inner, vertical_div_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_div_inner();
        compare.setKernel(5,2);
        compare.vertical_div_inner();
        compare.setKernel(7,3);
        compare.vertical_div_inner();
        compare.setKernel(7,1);
        compare.vertical_div_inner();
        compare.setKernel(7,4);
        compare.vertical_div_inner();
    }
}

TEST(ConvolveImage_Inner, vertical_div_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_div_inner();
        compare.setKernel(5,2);
        compare.vertical_div_inner();
        compare.setKernel(7,3);
        compare.vertical_div_inner();
        compare.setKernel(7,1);
        compare.vertical_div_inner();
        compare.setKernel(7,4);
        compare.vertical_div_inner();
    }
}

TEST(ConvolveImage_Inner, horizontal_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_inner();
        compare.setKernel(5,2);
        compare.horizontal_inner();
        compare.setKernel(7,3);
        compare.horizontal_inner();
        compare.setKernel(7,1);
        compare.horizontal_inner();
        compare.setKernel(7,4);
        compare.horizontal_inner();
    }
}

TEST(ConvolveImage_Inner, horizontal_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_inner();
        compare.setKernel(5,2);
        compare.horizontal_inner();
        compare.setKernel(7,3);
        compare.horizontal_inner();
        compare.setKernel(7,1);
        compare.horizontal_inner();
        compare.setKernel(7,4);
        compare.horizontal_inner();
    }
}

TEST(ConvolveImage_Inner, vertical_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_inner();
        compare.setKernel(5,2);
        compare.vertical_inner();
        compare.setKernel(7,3);
        compare.vertical_inner();
        compare.setKernel(7,1);
        compare.vertical_inner();
        compare.setKernel(7,4);
        compare.vertical_inner();
    }
}

TEST(ConvolveImage_Inner, vertical_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_inner();
        compare.setKernel(5,2);
        compare.vertical_inner();
        compare.setKernel(7,3);
        compare.vertical_inner();
        compare.setKernel(7,1);
        compare.vertical_inner();
        compare.setKernel(7,4);
        compare.vertical_inner();
    }
}

TEST(ConvolveImage_Inner, convolve_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel2(3,1,true);
        compare.convolve_inner();
        compare.setKernel2(5,2,true);
        compare.convolve_inner();
        compare.setKernel2(7,3,true);
        compare.convolve_inner();
        compare.setKernel2(7,1,true);
        compare.convolve_inner();
        compare.setKernel2(7,4,true);
        compare.convolve_inner();
    }
}

TEST(ConvolveImage_Inner, convolve_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel2(3,1,true);
        compare.convolve_inner();
        compare.setKernel2(5,2,true);
        compare.convolve_inner();
        compare.setKernel2(7,3,true);
        compare.convolve_inner();
        compare.setKernel2(7,1,true);
        compare.convolve_inner();
        compare.setKernel2(7,4,true);
        compare.convolve_inner();
    }
}

TEST(ConvolveImage_Inner, convolve_div_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel2(3,1);
        compare.convolve_div_inner();
        compare.setKernel2(5,2);
        compare.convolve_div_inner();
        compare.setKernel2(7,3);
        compare.convolve_div_inner();
        compare.setKernel2(7,1);
        compare.convolve_div_inner();
        compare.setKernel2(7,4);
        compare.convolve_div_inner();
    }
}

TEST(ConvolveImage_Inner, convolve_div_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel2(3,1);
        compare.convolve_div_inner();
        compare.setKernel2(5,2);
        compare.convolve_div_inner();
        compare.setKernel2(7,3);
        compare.convolve_div_inner();
        compare.setKernel2(7,1);
        compare.convolve_div_inner();
        compare.setKernel2(7,4);
        compare.convolve_div_inner();
    }
}

TEST(ConvolveImage_Border, horizontal_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.horizontal_border();
        compare.setKernel(5,2);
        compare.horizontal_border();
        compare.setKernel(7,3);
        compare.horizontal_border();
        compare.setKernel(7,1);
        compare.horizontal_border();
        compare.setKernel(7,4);
        compare.horizontal_border();
    }
}

TEST(ConvolveImage_Border, vertical_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.vertical_border();
        compare.setKernel(5,2);
        compare.vertical_border();
        compare.setKernel(7,3);
        compare.vertical_border();
        compare.setKernel(7,1);
        compare.vertical_border();
        compare.setKernel(7,4);
        compare.vertical_border();
    }
}

TEST(ConvolveImage_Border, convolve2D_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel2(3,1);
        compare.convolve_border();
        compare.setKernel2(5,2);
        compare.convolve_border();
        compare.setKernel2(7,3);
        compare.convolve_border();
        compare.setKernel2(7,1);
        compare.convolve_border();
        compare.setKernel2(7,4);
        compare.convolve_border();
    }
}


TEST(ConvolveNormalized, horizontal_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.normalized_horizontal();
        compare.setKernel(5,2);
        compare.normalized_horizontal();
        compare.setKernel(7,3);
        compare.normalized_horizontal();
        compare.setKernel(7,1);
        compare.normalized_horizontal();
        compare.setKernel(7,4);
        compare.normalized_horizontal();
    }
}

TEST(ConvolveNormalized, horizontal_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.normalized_horizontal();
        compare.setKernel(5,2);
        compare.normalized_horizontal();
        compare.setKernel(7,3);
        compare.normalized_horizontal();
        compare.setKernel(7,1);
        compare.normalized_horizontal();
        compare.setKernel(7,4);
        compare.normalized_horizontal();
    }
}

TEST(ConvolveNormalized, vertical_U8) {
    CompareToNaive<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.normalized_vertical();
        compare.setKernel(5,2);
        compare.normalized_vertical();
        compare.setKernel(7,3);
        compare.normalized_vertical();
        compare.setKernel(7,1);
        compare.normalized_vertical();
        compare.setKernel(7,4);
        compare.normalized_vertical();
    }
}

TEST(ConvolveNormalized, vertical_F32) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.normalized_vertical();
        compare.setKernel(5,2);
        compare.normalized_vertical();
        compare.setKernel(7,3);
        compare.normalized_vertical();
        compare.setKernel(7,1);
        compare.normalized_vertical();
        compare.setKernel(7,4);
        compare.normalized_vertical();
    }
}

TEST(ConvolveNormalized, convolve_with1D) {
    CompareToNaive<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setKernel(3,1);
        compare.normalized_convolve_1D();
        compare.setKernel(5,2);
        compare.normalized_convolve_1D();
        compare.setKernel(7,3);
        compare.normalized_convolve_1D();
        compare.setKernel(7,1);
        compare.normalized_convolve_1D();
        compare.setKernel(7,4);
        compare.normalized_convolve_1D();
    }
}