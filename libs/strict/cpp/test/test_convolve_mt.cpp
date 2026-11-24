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
            check_shuffled([&]( uint32_t scale ) {
                check_matches_single_threaded<E, R>(op, 64*scale, 32*scale, 5, 2);
            });
        }
    }
};

/// Integer and floating point images, which take different inner code
TEST_F(ConvolveImage_MTTest, U8_S16) { check_all<U8, S16>(); }
TEST_F(ConvolveImage_MTTest, F32) { check_all<F32, F32>(); }
