#include "gtest/gtest.h"
#include "image_blur.h"
#include "image_misc_ops.h"
#include "testing_utils.h"

#include <random>

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

    void setMeanRadius(uint32_t radius) {
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
        BlurImageOps::mean(input,found,kernel.width/2,tmp);

        check_equals(expected,found);
    }

    void gaussian( int32_t kernel_width ) {

        kernel = FactoryKernel::gaussian1D<signed_type>(-1,kernel_width);

        Gray<E> tmp(input.width,input.height);
        ConvolveNormalizedNaive::horizontal(kernel,input,tmp);
        ConvolveNormalizedNaive::vertical(kernel,tmp,expected);
        BlurImageOps::gaussian(input,found,-1,kernel_width,tmp);

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
        compare.setMeanRadius(1);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(2);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_horizontal_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(2);
        compare.horizontal_mean_inner();
        compare.setMeanRadius(5);
        compare.horizontal_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_U8) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean_inner();
        compare.setMeanRadius(2);
        compare.vertical_mean_inner();
        compare.setMeanRadius(5);
        compare.vertical_mean_inner();
    }
}

TEST(ConvolveImageMean, inner_vertical_F32) {
    CompareToNormalized<F32> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean_inner();
        compare.setMeanRadius(2);
        compare.vertical_mean_inner();
        compare.setMeanRadius(5);
        compare.vertical_mean_inner();
    }
}

TEST(ConvolveImageMean, horizonal) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.horizontal_mean();
        compare.setMeanRadius(2);
        compare.horizontal_mean();
        compare.setMeanRadius(5);
        compare.horizontal_mean();
    }
}

TEST(ConvolveImageMean, vertical) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 2; i++ ) {
        uint32_t w = 15+i;
        uint32_t h = 20+i;

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.vertical_mean();
        compare.setMeanRadius(2);
        compare.vertical_mean();
        compare.setMeanRadius(5);
        compare.vertical_mean();
    }
}

TEST(BlurImageOps, mean) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 3; i++ ) {
        uint32_t w = 3+i;
        uint32_t h = 4+i;
//        printf("mean i = %d  %dx%d\n",i,w,h);

        compare.setImageSize(w,h);
        compare.setMeanRadius(1);
        compare.mean();
        compare.setMeanRadius(2);
        compare.mean();
        compare.setMeanRadius(5);
        compare.mean();
    }
}

TEST(BlurImageOps, gaussian) {
    CompareToNormalized<U8> compare;

    for( uint32_t i = 0; i < 3; i++ ) {
        uint32_t w = 3+i;
        uint32_t h = 4+i;
//        printf("mean i = %d  %dx%d\n",i,w,h);

        compare.setImageSize(w,h);
        compare.gaussian(3);
        compare.gaussian(5);
        compare.gaussian(7);
        compare.gaussian(4);
    }
}

namespace {
    /// Brute force mean of the region around (x,y), reading pixels through the border, in double
    double naive_mean_border( const ImageBorder<F32>& border, int32_t x, int32_t y, int32_t rx, int32_t ry ) {
        double sum = 0;
        for (int32_t j = -ry; j <= ry; j++) {
            for (int32_t i = -rx; i <= rx; i++)
                sum += border.get(x + i, y + j);
        }
        return sum/((2*rx + 1)*(2*ry + 1));
    }

    /// Compares mean_border() against the brute force mean for every border type and a sub-image
    void mean_border_compare_to_naive( uint32_t width, uint32_t height, int32_t rx, int32_t ry ) {
        Gray<F32> input(width, height);
        std::mt19937 gen(0xBEEF + width*31 + height);
        ImageMiscOps::fill_uniform(input, 0.0f, 10.0f, gen);

        for (BorderType type : {REFLECT, EXTENDED, WRAP, ZERO}) {
            for (bool sub : {false, true}) {
                Gray<F32> in = sub ? create_subimage(input) : input.makeSubimage(0, 0, width, height);
                auto border = FactoryImageBorder::create_SB<F32>(type);
                Gray<F32> output(1, 1), storage(1, 1);
                std::vector<F32> totals;
                BlurImageOps::mean_border(in, output, rx, ry, *border, storage, totals);

                auto expected_border = FactoryImageBorder::create_SB<F32>(type);
                expected_border->setImage(input);
                ASSERT_EQ(width, output.width);
                ASSERT_EQ(height, output.height);
                for (int32_t y = 0; y < (int32_t)height; y++) {
                    for (int32_t x = 0; x < (int32_t)width; x++) {
                        ASSERT_NEAR(naive_mean_border(*expected_border, x, y, rx, ry), output.at(x, y), 1e-5)
                                << width << "x" << height << " r=" << rx << "," << ry << " border=" << type
                                << " sub=" << sub << " at " << x << "," << y;
                    }
                }
                if (sub)
                    in.subimage = false;
            }
        }
    }
}

TEST(BlurImageOps, mean_border_compare_to_naive) {
    // the inner image, the border, and images smaller than the region
    for (auto shape : {std::pair{30u, 25u}, {8u, 9u}, {5u, 3u}, {2u, 7u}, {1u, 1u}}) {
        mean_border_compare_to_naive(shape.first, shape.second, 1, 1);
        mean_border_compare_to_naive(shape.first, shape.second, 3, 2);
        mean_border_compare_to_naive(shape.first, shape.second, 2, 4);
    }
}

TEST(BlurImageOps, mean_border_bad_radius) {
    Gray<F32> input(10, 8), output(1, 1), storage(1, 1);
    std::vector<F32> totals;
    auto border = FactoryImageBorder::create_SB<F32>(REFLECT);
    EXPECT_THROW(BlurImageOps::mean_border(input, output, 0, 2, *border, storage, totals), invalid_argument);
    EXPECT_THROW(BlurImageOps::mean_border(input, output, 2, 0, *border, storage, totals), invalid_argument);
}
