#include <cstring>
#include "boofcv/print_structures.h"
#include "gtest/gtest.h"
#include "boofcv/convolve.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"
#include "variant_strict.h"
#include "shared/convolve_checks.h"
#include "shared/convolve_normalized_checks.h"

using namespace std;
using namespace boofcv;

template<class E>
class CompareToNaive {
public:
    typedef typename TypeInfo<E>::signed_type signed_type;

    Gray<E> input,found,expected;
    Kernel1D<signed_type> kernel;
    Kernel2D<signed_type> kernel2;

    std::mt19937& rng;

    uint32_t borderX0,borderX1;
    uint32_t borderY0,borderY1;

    explicit CompareToNaive( std::mt19937& rng ) : rng(rng) {}

    void setImageSize( uint32_t width , uint32_t height ) {

        input.reshape(width,height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)50,rng);
        found.reshape(width,height);
        expected.reshape(width,height);
    }

    /// Without negative weights, they're at least 1, so that the normalized border code never divides by zero
    void setKernel( uint32_t width , uint32_t offset , bool has_negative=false) {
        kernel.reshape(width,offset);
        if( has_negative )
            KernelOps::fill_uniform(kernel,(signed_type)-10,(signed_type)10,rng);
        else
            KernelOps::fill_uniform(kernel,(signed_type)1,(signed_type)10,rng);
    }

    void setKernel2( uint32_t width , uint32_t offset , bool has_negative=false) {
        kernel2.reshape(width,offset);
        if( has_negative )
            KernelOps::fill_uniform(kernel2,(signed_type)-10,(signed_type)10,rng);
        else
            KernelOps::fill_uniform(kernel2,(signed_type)1,(signed_type)10,rng);
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

    void convolve_norm_border() {
        ConvolveNormalizedNaive::convolve(kernel2,input,expected);
        ConvolveNormalized_JustBorder::convolve(kernel2,input,found);

        borderX0 = borderY0 = kernel2.offset;
        borderX1 = borderY1 = kernel2.width-1-kernel2.offset;

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

        // Floating point inner convolution sums one kernel row at a time to match BoofCV, which is a different
        // order than the naive implementation. Tolerance is made relative to the magnitude of the output.
        // Integer tolerance stays zero.
        double max_abs = 1.0;
        for( uint32_t i = 0; i < expected.total_pixels(); i++ )
            max_abs = std::max(max_abs, (double)std::abs((double)expected.data[i]));
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1,
                           (E)(test_tolerance<E>()*max_abs));
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





    void checkResults_border() {
        check_equals_border(expected,found,borderX0,borderX1,borderY0,borderY1);
    }

    void checkResults_inner() {
        check_equals_inner(expected,found,borderX0,borderX1,borderY0,borderY1);
    }

};

class ConvolveNormalized_JustBorderTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveNormalized_JustBorderTest, horizontal_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveNormalized_JustBorderTest, horizontal_F32) {
    CompareToNaive<F32> compare(rng);

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

TEST_F(ConvolveNormalized_JustBorderTest, vertical_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveNormalized_JustBorderTest, vertical_F32) {
    CompareToNaive<F32> compare(rng);

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

/// Rows longer than the number of pixels the border code sums at once, and kernels that aren't centered
template<class E>
void check_just_border_wide( std::mt19937& rng ) {
    CompareToNaive<E> compare(rng);
    for (uint32_t w : {255u, 256u, 257u, 600u}) {
        compare.setImageSize(w, 9);
        for (auto [width, offset] : {std::pair{5u, 2u}, {7u, 1u}, {4u, 3u}, {7u, 6u}}) {
            compare.setKernel(width, offset);
            compare.horizontal_norm_border();
            compare.vertical_norm_border();
            compare.setKernel2(width, offset);
            // a corner's window can have one weight, which must not be zero for integers
            for (uint32_t i = 0; i < compare.kernel2.data.size; i++)
                if (compare.kernel2.data[i] == 0)
                    compare.kernel2.data[i] = 1;
            compare.convolve_norm_border();
        }
    }
}

/// vertical() with a row range writes the top and bottom border rows in it, and nothing else
TEST_F(ConvolveNormalized_JustBorderTest, vertical_range_U8) {
    const uint32_t w = 13, h = 20;
    Gray<U8> input(w, h), expected(w, h), found(w, h);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)200, rng);
    Kernel1D<S32> kernel(7, 2);
    KernelOps::fill_uniform(kernel, 1, 10, rng);
    ImageMiscOps::fill(expected, (U8)251);
    ConvolveNormalized_JustBorder::vertical(kernel, input, expected);
    const uint32_t ranges[][2] = {{0, h}, {0, 1}, {1, 3}, {2, 17}, {5, 15}, {16, 18}, {19, h}};
    for (auto& r : ranges) {
        ImageMiscOps::fill(found, (U8)251);
        ConvolveNormalized_JustBorder::vertical(kernel, input, found, r[0], r[1]);
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                const bool inside = y >= r[0] && y < r[1];
                ASSERT_EQ(inside ? expected.at(x, y) : 251, found.at(x, y)) << x << " " << y;
            }
        }
    }
}

TEST_F(ConvolveNormalized_JustBorderTest, wide_U8) { check_just_border_wide<U8>(rng); }

TEST_F(ConvolveNormalized_JustBorderTest, wide_F32) { check_just_border_wide<F32>(rng); }

class ConvolveImage_InnerTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveImage_InnerTest, horizontal_div_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, horizontal_div_F32) {
    CompareToNaive<F32> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, vertical_div_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, vertical_div_F32) {
    CompareToNaive<F32> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, horizontal_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, horizontal_F32) {
    CompareToNaive<F32> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, vertical_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, vertical_F32) {
    CompareToNaive<F32> compare(rng);

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

/// vertical_rows_registers is bit identical to vertical_rows: kernels that aren't centered, an input sub-image, and
/// image widths below, at and above vertical_rows' block of 256 columns
template<int W, class E>
void check_vertical_registers( std::mt19937& rng ) {
    typedef typename TypeInfo<E>::signed_type K;
    auto result = []( K total ) { return (E)(total/(K)7); };
    for( uint32_t width : {1u, 17u, 256u, 300u} ) {
        Gray<E> parent(width+5, 3*W);
        ImageMiscOps::fill_uniform(parent, (E)0, (E)50, rng);
        Gray<E> input = parent.makeSubimage(2, 1, 2+width, parent.height-1);
        for( uint32_t offset : {(uint32_t)W/2, 0u, (uint32_t)W-1} ) {
            Kernel1D<K> kernel(W, offset);
            KernelOps::fill_uniform(kernel, (K)-10, (K)10, rng);
            Gray<E> expected(width, input.height), found(width, input.height);
            ImageMiscOps::fill(expected, (E)1);
            ImageMiscOps::fill(found, (E)1);
            ConvolveImage_Inner::vertical_rows<W>(kernel, input, expected, result);
            ConvolveImage_Inner::vertical_rows_registers<W>(kernel, input, found, result);
            for( uint32_t y = 0; y < input.height; y++ )
                ASSERT_EQ(0, std::memcmp(&expected.data[y*expected.stride], &found.data[y*found.stride],
                                         width*sizeof(E))) << "W " << W << " width " << width << " offset " << offset;
        }
    }
}

template<class E>
void check_vertical_registers( std::mt19937& rng ) {
    check_vertical_registers<3,E>(rng);
    check_vertical_registers<5,E>(rng);
    check_vertical_registers<7,E>(rng);
    check_vertical_registers<9,E>(rng);
}

/// vertical_unrolled on both sides of the switch between vertical_rows_registers (floats, widths 3 to 9) and
/// vertical_rows (wider), with and without a divisor, against the naive convolution
template<class E>
void check_vertical_unrolled_widths( std::mt19937& rng ) {
    typedef typename TypeInfo<E>::signed_type K;
    Gray<E> input(37, 40);
    ImageMiscOps::fill_uniform(input, (E)0, (E)50, rng);
    ImageBorderValue<E> border(0);
    border.setImage(input);
    for( uint32_t width : {3u, 5u, 7u, 9u, 11u, 13u, 15u} ) {
        for( uint32_t offset : {width/2, 0u, width-1} ) {
            Kernel1D<K> kernel(width, offset);
            KernelOps::fill_uniform(kernel, (K)-10, (K)10, rng);
            Gray<E> expected(input.width, input.height), found(input.width, input.height);
            ConvolveNaive::vertical(kernel, border, expected);
            ConvolveImage_Inner::vertical_unrolled(kernel, input, found);
            check_equals_inner(expected, found, 0, 0, offset, width-1-offset);
            ConvolveNaive::vertical(kernel, border, expected, (K)7);
            ConvolveImage_Inner::vertical_unrolled(kernel, input, found, (K)7);
            check_equals_inner(expected, found, 0, 0, offset, width-1-offset);
        }
    }
}

TEST_F(ConvolveImage_InnerTest, vertical_unrolled_F32) { check_vertical_unrolled_widths<F32>(rng); }

TEST_F(ConvolveImage_InnerTest, vertical_unrolled_F64) { check_vertical_unrolled_widths<F64>(rng); }

TEST_F(ConvolveImage_InnerTest, vertical_registers_F32) { check_vertical_registers<F32>(rng); }

TEST_F(ConvolveImage_InnerTest, vertical_registers_F64) { check_vertical_registers<F64>(rng); }

TEST_F(ConvolveImage_InnerTest, convolve_U8) {
    CompareToNaive<U8> compare(rng);

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

/// Integer images with a wider output type, for every kernel width with unrolled code and a kernel that isn't centered.
/// Pixels and weights are small enough that no sum overflows S16, 13*13*50*3 = 25350 at most
TEST_F(ConvolveImage_InnerTest, convolve_U8_S16) {
    Gray<U8> input(30,25);
    ImageMiscOps::fill_uniform(input, (U8)0, (U8)50, rng);
    ImageBorderValue<U8> border(0);
    border.setImage(input);

    for( uint32_t width : {3u, 5u, 7u, 9u, 11u, 13u} ) {
        for( uint32_t offset : {width/2, 0u} ) {
            Kernel2D<S32> kernel(width,offset);
            KernelOps::fill_uniform(kernel,(S32)-3,(S32)3,rng);
            Gray<S16> expected(input.width,input.height), found(input.width,input.height);
            ConvolveNaive::convolve(kernel,border,expected);
            ConvolveImage_Inner::convolve(kernel,input,found);
            check_equals_inner(expected,found,offset,width-1-offset,offset,width-1-offset);
        }
    }
}

TEST_F(ConvolveImage_InnerTest, convolve_F32) {
    CompareToNaive<F32> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, convolve_div_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_InnerTest, convolve_div_F32) {
    CompareToNaive<F32> compare(rng);

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

class ConvolveImage_BorderTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveImage_BorderTest, horizontal_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_BorderTest, vertical_U8) {
    CompareToNaive<U8> compare(rng);

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

TEST_F(ConvolveImage_BorderTest, convolve2D_U8) {
    CompareToNaive<U8> compare(rng);

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


class ConvolveNormalizedTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveNormalizedTest, U8) { convolve_normalized_checks::check_all<U8>(rng); }
TEST_F(ConvolveNormalizedTest, U16) { convolve_normalized_checks::check_all<U16>(rng); }
TEST_F(ConvolveNormalizedTest, F32) { convolve_normalized_checks::check_all<F32>(rng); }
TEST_F(ConvolveNormalizedTest, F64) { convolve_normalized_checks::check_all<F64>(rng); }

class ConvolveImageTest : public boofcv::BoofCPPTest {};

TEST_F(ConvolveImageTest, U8) { convolve_checks::check_all<U8>(rng); }
TEST_F(ConvolveImageTest, U8_S16) { convolve_checks::check_all<U8, S16>(rng); }
TEST_F(ConvolveImageTest, U8_S32) { convolve_checks::check_all<U8, S32>(rng); }
TEST_F(ConvolveImageTest, F32) { convolve_checks::check_all<F32>(rng); }
TEST_F(ConvolveImageTest, F64) { convolve_checks::check_all<F64>(rng); }

/**
 * Kernel is only normalized when its sum isn't within 1e-4 of one, same as BoofCV
 */
TEST_F(ConvolveNormalizedTest, needs_normalization) {
    Kernel1D<F32> k(3,1);
    k.data[0] = 0.25f; k.data[1] = 0.5f; k.data[2] = 0.25f;
    EXPECT_FALSE(ConvolveNormalized::needs_normalization(k));
    k.data[1] = 0.50009f;
    EXPECT_FALSE(ConvolveNormalized::needs_normalization(k));
    k.data[1] = 0.5002f;
    EXPECT_TRUE(ConvolveNormalized::needs_normalization(k));

    k.data[0] = 1.0f; k.data[1] = 2.0f; k.data[2] = 1.0f;
    ASSERT_TRUE(ConvolveNormalized::needs_normalization(k));
    Kernel1D<F32> n(3,1);
    ConvolveNormalized::normalize_sum_to_one(k,n);
    EXPECT_EQ(0.25f,n.data[0]);
    EXPECT_EQ(0.5f,n.data[1]);
    EXPECT_EQ(0.25f,n.data[2]);
    EXPECT_EQ(1u,n.offset);
}
