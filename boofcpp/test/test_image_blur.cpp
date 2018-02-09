#include "gtest/gtest.h"
#include "image_blur.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

#include "print_structures.h"

using namespace std;
using namespace boofcv;

template<class E>
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
        kernel = FactoryKernel::mean1D<signed_type>(radius*2+1);
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

    void horizontal_mean() {
        ConvolveNormalizedNaive::horizontal(kernel,input,expected);
        ConvolveImageMean::horizontal(input,found,kernel.width/2);

        check_equals(expected,found);
    }

    void vertical_mean() {
        ConvolveNormalizedNaive::vertical(kernel,input,expected);
        ConvolveImageMean::vertical(input,found,kernel.width/2);

        check_equals(expected,found);
    }

    void mean() {
        Gray<E> tmp(input.width,input.height);
        ConvolveNormalizedNaive::horizontal(kernel,input,tmp);
        ConvolveNormalizedNaive::vertical(kernel,tmp,expected);
        ConvolveImageMean::mean(input,found,kernel.width/2,tmp);

        check_equals(expected,found);
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

TEST(ConvolveImageMean, horizonal) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.horizontal_mean();
        compare.setBlurRadius(2);
        compare.horizontal_mean();
        compare.setBlurRadius(5);
        compare.horizontal_mean();
    }
}

TEST(ConvolveImageMean, vertical) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.vertical_mean();
        compare.setBlurRadius(2);
        compare.vertical_mean();
        compare.setBlurRadius(5);
        compare.vertical_mean();
    }
}

TEST(ConvolveImageMean, mean) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 3; i++ ) {
        uint32_t w = 3+i;
        uint32_t h = 4+i;
//        printf("mean i = %d  %dx%d\n",i,w,h);

        compare.setImageSize(w,h);
        compare.setBlurRadius(1);
        compare.mean();
        compare.setBlurRadius(2);
        compare.mean();
        compare.setBlurRadius(5);
        compare.mean();
    }
}
