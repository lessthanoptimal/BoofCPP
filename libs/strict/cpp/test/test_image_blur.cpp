#include "gtest/gtest.h"
#include "boofcv/image_blur.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <random>

#include "boofcv/print_structures.h"

using namespace std;
using namespace boofcv;

// BlurImageOps::mean, mean_border and gaussian are tested by checks shared with the other variants
#include "variant_strict.h"
#include "shared/blur_mean_checks.h"
#include "shared/blur_mean_border_checks.h"
#include "shared/blur_gaussian_checks.h"

class BlurImageOpsTest : public boofcv::BoofCPPTest {};

TEST_F(BlurImageOpsTest, mean_U8) { blur_mean_checks::check_mean_all<U8>(rng); }
TEST_F(BlurImageOpsTest, mean_U16) { blur_mean_checks::check_mean_all<U16>(rng); }
TEST_F(BlurImageOpsTest, mean_F32) { blur_mean_checks::check_mean_all<F32>(rng); }
TEST_F(BlurImageOpsTest, mean_F64) { blur_mean_checks::check_mean_all<F64>(rng); }
TEST_F(BlurImageOpsTest, mean_bad_radius) { blur_mean_checks::check_mean_bad_radius<F32>(); }

template<class E>
class CompareToNormalized {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;

    std::mt19937& rng;

    uint32_t borderX0,borderX1;
    uint32_t borderY0,borderY1;

    explicit CompareToNormalized( std::mt19937& rng ) : rng(rng) {}

    void setImageSize( uint32_t width , uint32_t height ) {

        input.reshape(width,height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)50,rng);
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

    void checkResults_inner() {
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1);
    }
};

class ConvolveImageMeanTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveImageMeanTest, inner_horizontal_U8) {
    CompareToNormalized<U8> compare(rng);

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

TEST_F(ConvolveImageMeanTest, inner_horizontal_F32) {
    CompareToNormalized<F32> compare(rng);

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

TEST_F(ConvolveImageMeanTest, inner_vertical_U8) {
    CompareToNormalized<U8> compare(rng);

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

TEST_F(ConvolveImageMeanTest, inner_vertical_F32) {
    CompareToNormalized<F32> compare(rng);

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

TEST_F(ConvolveImageMeanTest, horizonal) {
    CompareToNormalized<U8> compare(rng);

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

TEST_F(ConvolveImageMeanTest, vertical) {
    CompareToNormalized<U8> compare(rng);

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

TEST_F(BlurImageOpsTest, gaussian_U8) { blur_gaussian_checks::check_all<U8>(rng); }
TEST_F(BlurImageOpsTest, gaussian_U16) { blur_gaussian_checks::check_all<U16>(rng); }
TEST_F(BlurImageOpsTest, gaussian_F32) { blur_gaussian_checks::check_all<F32>(rng); }
TEST_F(BlurImageOpsTest, gaussian_F64) { blur_gaussian_checks::check_all<F64>(rng); }

TEST_F(BlurImageOpsTest, mean_border_U8) { blur_mean_border_checks::check_all<U8>(rng, 0, 255); }
// negative values check the rounding of negative sums
TEST_F(BlurImageOpsTest, mean_border_S16) { blur_mean_border_checks::check_all<S16>(rng, -30000, 30000); }
TEST_F(BlurImageOpsTest, mean_border_U16) { blur_mean_border_checks::check_all<U16>(rng, 0, 65535); }
TEST_F(BlurImageOpsTest, mean_border_F32) { blur_mean_border_checks::check_all<F32>(rng, 0, 10); }
TEST_F(BlurImageOpsTest, mean_border_F64) { blur_mean_border_checks::check_all<F64>(rng, 0, 10); }
TEST_F(BlurImageOpsTest, mean_border_bad_radius) { blur_mean_border_checks::check_bad_radius<F32>(); }
