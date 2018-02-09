#include "gtest/gtest.h"
#include "image_blur.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

#include <stdio.h>
#include "sanity_checks.h"

using namespace std;
using namespace boofcv;

template<typename E>
void print_difference( const Gray<E>& a , const Gray<E>& b ) {
    checkSameShape(a,b);

    for( uint32_t y = 0; y < a.height; y++ ) {
        for( uint32_t x = 0; x < a.width; x++ ) {
            if( a.at(x,y) != b.at(x,y)) {
                printf("X");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
}

template<typename E>
void print_kernel( const Kernel1D<E>& a , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {

    printf("Kernel1D[");
    for( uint32_t y = 0; y < a.width; y++ ) {
        printf(" %d",a[y]);
    }
    printf(" ]\n");
}

template<class E, class Enable = void>
class CompareToNormalized {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;

    std::mt19937 gen;

    uint32_t borderX0,borderX1;
    uint32_t borderY0,borderY1;

    CompareToNormalized() : gen(0xBEEF) {

    }

    void setImageSize( uint32_t width , uint32_t height ) {

        input.reshape(width,height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)50,gen);
        found.reshape(width,height);
        expected.reshape(width,height);
    }

    void setBlurRadius( uint32_t radius ) {
        kernel = FactoryKernel::table1D<signed_type>(radius*2+1);
        FactoryKernel::normalize(kernel);
    }

    void horizontal_mean_inner() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveImageMean::inner_horizontal(input,found,kernel.width/2);

        borderY0=borderY1=0;
        borderX0 = kernel.offset;
        borderX1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void vertical_mean_inner() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveImageMean::inner_vertical(input,found,kernel.width/2);

        borderX0=borderX1=0;
        borderY0 = kernel.offset;
        borderY1 = kernel.width-1-kernel.offset;

        checkResults_inner();
    }

    void checkResults_inner() {
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1);
    }
};

TEST(ConvolveImageMean, inner_horizontal_U8) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.horizontal_mean_inner();
        compare.setBlurRadius(2);
        compare.horizontal_mean_inner();
        compare.setBlurRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_horizontal_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.horizontal_mean_inner();
        compare.setBlurRadius(2);
        compare.horizontal_mean_inner();
        compare.setBlurRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_U8) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.vertical_mean_inner();
        compare.setBlurRadius(2);
        compare.vertical_mean_inner();
        compare.setBlurRadius(5);
        compare.vertical_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.vertical_mean_inner();
        compare.setBlurRadius(2);
        compare.vertical_mean_inner();
        compare.setBlurRadius(5);
        compare.vertical_mean_inner();
    }
}

