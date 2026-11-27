#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/convolve_mt.h"
#include "boofcv/convolve_kernels.h"
#include "boofcv/image_misc_ops.h"

#include <memory>
#include <string>

using namespace std;
using namespace boofcv;

/// Calls 'check(width, height, kernel_width, offset)' for the cases that matter to _MT code, which runs the single
/// threaded code on blocks of rows: fewer rows than the kernel's border, several blocks with a kernel that isn't
/// centered, and a kernel as large as the image
template<class Check>
void for_each_case( Check check ) {
    check(41, 2, 5, 1);
    check(41, 23, 5, 1);
    check(7, 7, 7, 3);
}

/// ConvolveImage_MT must be identical to ConvolveImage for any number of threads
class ConvolveImage_MTTest : public boofcv::ConcurrentTest {
protected:
    enum class Op { HORIZONTAL, VERTICAL, CONVOLVE };

    /// Compares one operation with the current settings, on a sub-image input and output and with kernel weights
    /// that can be negative. The border type doesn't matter to the _MT code, which only splits the border into rows
    template<class E, class R>
    void check_matches_single_threaded( Op op, uint32_t width, uint32_t height, uint32_t kernel_width,
                                        uint32_t offset ) {
        typedef typename TypeInfo<E>::signed_type K;
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)100, rng);
        shared_ptr<ImageBorder<E>> border = FactoryImageBorder::create_SB<E>(REFLECT);
        border->setImage(input);

        Gray<R> expected(width, height);
        Gray<R> found_parent;
        Gray<R> found = subimage(found_parent, width, height);
        ImageMiscOps::fill(found, (R)1);

        Kernel1D<K> kernel1(kernel_width, offset);
        Kernel2D<K> kernel2(kernel_width, offset);
        KernelOps::fill_uniform(kernel1, (K)-10, (K)10, rng);
        KernelOps::fill_uniform(kernel2, (K)-10, (K)10, rng);
        switch (op) {
            case Op::HORIZONTAL:
                ConvolveImage::horizontal(kernel1, *border, expected);
                ConvolveImage_MT::horizontal(kernel1, *border, found);
                break;
            case Op::VERTICAL:
                ConvolveImage::vertical(kernel1, *border, expected);
                ConvolveImage_MT::vertical(kernel1, *border, found);
                break;
            case Op::CONVOLVE:
                ConvolveImage::convolve(kernel2, *border, expected);
                ConvolveImage_MT::convolve(kernel2, *border, found);
                break;
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " kernel " +
                                         to_string(kernel_width) + " offset " + to_string(offset));
    }

    template<class E, class R>
    void check_all() {
        for (Op op : {Op::HORIZONTAL, Op::VERTICAL, Op::CONVOLVE}) {
            for_each_setting([&] {
                for_each_case([&]( uint32_t width, uint32_t height, uint32_t kernel_width, uint32_t offset ) {
                    check_matches_single_threaded<E, R>(op, width, height, kernel_width, offset);
                });
            });
            check_blocks_out_of_order([&]( uint32_t scale ) {
                check_matches_single_threaded<E, R>(op, 64*scale, 32*scale, 5, 2);
            });
        }
    }
};

/// Integer and floating point images, which take different inner code
TEST_F(ConvolveImage_MTTest, U8_S16) { check_all<U8, S16>(); }
TEST_F(ConvolveImage_MTTest, F32) { check_all<F32, F32>(); }

/// ConvolveNormalized_MT must be identical to ConvolveNormalized for any number of threads
class ConvolveNormalized_MTTest : public boofcv::ConcurrentTest {
protected:
    enum class Op { HORIZONTAL, VERTICAL, CONVOLVE };

    /// Compares one operation with the current settings, on a sub-image input and output. Weights are positive, so
    /// the weights inside the image never sum to zero. Float kernels that sum to one aren't rescaled, the others are
    template<class E>
    void check_matches_single_threaded( Op op, uint32_t width, uint32_t height, uint32_t kernel_width,
                                        uint32_t offset, bool sum_to_one = false ) {
        typedef typename TypeInfo<E>::signed_type K;
        Gray<E> parent, found_parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)100, rng);
        Gray<E> expected(width, height);
        Gray<E> found = subimage(found_parent, width, height);
        ImageMiscOps::fill(found, (E)1);

        Kernel1D<K> kernel1(kernel_width, offset);
        Kernel2D<K> kernel2(kernel_width, offset);
        KernelOps::fill_uniform(kernel1, (K)1, (K)10, rng);
        KernelOps::fill_uniform(kernel2, (K)1, (K)10, rng);
        if (sum_to_one) {
            FactoryKernel::normalize(kernel1);
            FactoryKernel::normalize(kernel2);
        }
        switch (op) {
            case Op::HORIZONTAL:
                ConvolveNormalized::horizontal(kernel1, input, expected);
                ConvolveNormalized_MT::horizontal(kernel1, input, found);
                break;
            case Op::VERTICAL:
                ConvolveNormalized::vertical(kernel1, input, expected);
                ConvolveNormalized_MT::vertical(kernel1, input, found);
                break;
            case Op::CONVOLVE:
                ConvolveNormalized::convolve(kernel2, input, expected);
                ConvolveNormalized_MT::convolve(kernel2, input, found);
                break;
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " kernel " +
                                         to_string(kernel_width) + " offset " + to_string(offset));
    }

    /// for_each_case(), and for floats a kernel that sums to one, which isn't rescaled
    template<class E>
    void check_all() {
        for (Op op : {Op::HORIZONTAL, Op::VERTICAL, Op::CONVOLVE}) {
            for_each_setting([&] {
                for_each_case([&]( uint32_t width, uint32_t height, uint32_t kernel_width, uint32_t offset ) {
                    check_matches_single_threaded<E>(op, width, height, kernel_width, offset);
                });
                if (std::is_floating_point<E>::value)
                    check_matches_single_threaded<E>(op, 41, 23, 5, 1, true);
            });
            check_blocks_out_of_order([&]( uint32_t scale ) {
                check_matches_single_threaded<E>(op, 64*scale, 32*scale, 5, 2);
            });
        }
    }
};

/// Integer and floating point images, which take different inner code
TEST_F(ConvolveNormalized_MTTest, U8) { check_all<U8>(); }
TEST_F(ConvolveNormalized_MTTest, F32) { check_all<F32>(); }

/// Integer kernels whose weights sum to zero throw, as the single threaded code does
TEST_F(ConvolveNormalized_MTTest, weights_sum_to_zero) {
    BoofConcurrency::set_max_threads(many_threads());
    Gray<U8> input(10, 9), output(10, 9);
    ImageMiscOps::fill(input, (U8)5);
    Kernel1D<S32> kernel1(3, 1);
    for (uint32_t i = 0; i < kernel1.data.size; i++)
        kernel1.data[i] = 0;
    EXPECT_THROW(ConvolveNormalized_MT::horizontal(kernel1, input, output), domain_error);
    EXPECT_THROW(ConvolveNormalized_MT::vertical(kernel1, input, output), domain_error);
    Kernel2D<S32> kernel2(3, 1);
    for (uint32_t i = 0; i < kernel2.data.size; i++)
        kernel2.data[i] = i == 0 ? 1 : (i == 1 ? -1 : 0);
    EXPECT_THROW(ConvolveNormalized_MT::convolve(kernel2, input, output), domain_error);
}
