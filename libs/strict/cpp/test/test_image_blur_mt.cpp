#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_blur_mt.h"
#include "boofcv/image_misc_ops.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace boofcv;

/// The _MT mean filters and Gaussian blur must be identical to the single threaded ones for any number of threads
class BlurImageOps_MTTest : public boofcv::ConcurrentTest {
protected:
    enum class Op { MEAN, MEAN_WITH_BORDER, HORIZONTAL_NORMALIZED, VERTICAL_NORMALIZED, HORIZONTAL_IMAGE_BORDER,
                    VERTICAL_IMAGE_BORDER };

    /// Compares one operation with the current settings, on a sub-image input and output. The _MT code only splits
    /// the rows, so the border type doesn't matter to it
    template<class E>
    void check_matches_single_threaded( Op op, uint32_t width, uint32_t height, uint32_t radius_x,
                                        uint32_t radius_y ) {
        Gray<E> parent, found_parent, storage;
        Gray<E> input = subimage(parent, width, height);
        const E max_value = std::is_floating_point<E>::value ? (E)1000 :
                            (E)std::min(1000, (int)std::numeric_limits<E>::max());
        ImageMiscOps::fill_uniform(input, (E)0, max_value, rng);
        Gray<E> expected(width, height);
        Gray<E> found = subimage(found_parent, width, height);
        ImageMiscOps::fill(found, (E)13);
        shared_ptr<ImageBorder<E>> border = FactoryImageBorder::create_SB<E>(REFLECT);
        vector<typename TypeInfo<E>::sum_type> totals;
        switch (op) {
            case Op::MEAN:
                BlurImageOps::mean(input, expected, radius_x, radius_y, storage);
                BlurImageOps_MT::mean(input, found, radius_x, radius_y, storage);
                break;
            case Op::MEAN_WITH_BORDER:
                BlurImageOps::mean(input, expected, radius_x, radius_y, *border, storage, totals);
                BlurImageOps_MT::mean(input, found, radius_x, radius_y, *border, storage);
                break;
            case Op::HORIZONTAL_NORMALIZED:
                ConvolveImageMean::horizontal(input, expected, radius_x);
                ConvolveImageMean_MT::horizontal(input, found, radius_x);
                break;
            case Op::VERTICAL_NORMALIZED:
                ConvolveImageMean::vertical(input, expected, radius_y);
                ConvolveImageMean_MT::vertical(input, found, radius_y);
                break;
            case Op::HORIZONTAL_IMAGE_BORDER:
                ConvolveImageMean::horizontal(input, expected, radius_x, 2*radius_x + 1, *border);
                ConvolveImageMean_MT::horizontal(input, found, radius_x, 2*radius_x + 1, *border);
                break;
            case Op::VERTICAL_IMAGE_BORDER:
                ConvolveImageMean::vertical(input, expected, radius_y, 2*radius_y + 1, *border, totals);
                ConvolveImageMean_MT::vertical(input, found, radius_y, 2*radius_y + 1, *border);
                break;
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " radius " +
                                         to_string(radius_x) + "," + to_string(radius_y));
    }

    /// Image sizes smaller and larger than the kernels, heights not a multiple of the blocks, and radius_x and
    /// radius_y that differ
    template<class E>
    void check_all() {
        for (Op op : {Op::MEAN, Op::MEAN_WITH_BORDER, Op::HORIZONTAL_NORMALIZED, Op::VERTICAL_NORMALIZED,
                      Op::HORIZONTAL_IMAGE_BORDER, Op::VERTICAL_IMAGE_BORDER}) {
            for_each_setting([&] {
                for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(17u, 9u),
                                  make_pair(41u, 23u), make_pair(64u, 64u), make_pair(300u, 157u)}) {
                    for (auto radius : {make_pair(1u, 1u), make_pair(2u, 5u), make_pair(12u, 3u)})
                        check_matches_single_threaded<E>(op, size.first, size.second, radius.first, radius.second);
                }
            });
            check_shuffled([&]( uint32_t scale ) {
                check_matches_single_threaded<E>(op, 64*scale, 32*scale, 2, 3);
            });
        }
    }

    /// gaussian() with the same and with different kernels along x and y, on an image with several blocks. It only
    /// calls ConvolveNormalized_MT, whose tests cover the image sizes and blocks
    template<class E>
    void check_gaussian() {
        for_each_setting([&] {
            Gray<E> parent, storage, expected, found;
            Gray<E> input = subimage(parent, 41, 23);
            ImageMiscOps::fill_uniform(input, (E)0, (E)100, rng);
            BlurImageOps::gaussian(input, expected, 1.5, 7, -1, 5, storage);
            BlurImageOps_MT::gaussian(input, found, 1.5, 7, -1, 5, storage);
            check_identical(expected, found, "different kernels");
            BlurImageOps::gaussian(input, expected, -1, 5, storage);
            BlurImageOps_MT::gaussian(input, found, -1, 5, storage);
            check_identical(expected, found, "same kernel");
        });
    }
};

TEST_F(BlurImageOps_MTTest, U8) { check_all<U8>(); }
TEST_F(BlurImageOps_MTTest, U16) { check_all<U16>(); }
/// Floating point blocks along y start on the rows where the single threaded code restarts its sums
TEST_F(BlurImageOps_MTTest, F32) { check_all<F32>(); }
TEST_F(BlurImageOps_MTTest, F64) { check_all<F64>(); }

/// Integer and floating point images, which take different inner code
TEST_F(BlurImageOps_MTTest, gaussian_U8) { check_gaussian<U8>(); }
TEST_F(BlurImageOps_MTTest, gaussian_F32) { check_gaussian<F32>(); }
