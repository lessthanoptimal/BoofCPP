#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_blur_mt.h"
#include "boofcv/image_misc_ops.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace boofcv;

/// The _MT mean filters must be identical to the single threaded ones for any number of threads
class BlurImageOps_MTTest : public boofcv::ConcurrentTest {
protected:
    enum class Op { MEAN, MEAN_BORDER, HORIZONTAL_NORMALIZED, VERTICAL_NORMALIZED, HORIZONTAL_IMAGE_BORDER,
                    VERTICAL_IMAGE_BORDER };

    /// Compares one operation with the current settings, on a sub-image input and output. The _MT code only splits
    /// the rows, so the border type doesn't matter to it
    template<class E>
    void check_matches_single_threaded( Op op, uint32_t width, uint32_t height, uint32_t radius_x,
                                        uint32_t radius_y ) {
        Gray<E> parent, found_parent, storage;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)std::min(1000, (int)std::numeric_limits<E>::max()), rng);
        Gray<E> expected(width, height);
        Gray<E> found = subimage(found_parent, width, height);
        ImageMiscOps::fill(found, (E)13);
        shared_ptr<ImageBorder<E>> border = FactoryImageBorder::create_SB<E>(REFLECT);
        vector<typename TypeInfo<E>::sum_type> totals;
        const auto rx = (int32_t)radius_x, ry = (int32_t)radius_y;
        switch (op) {
            case Op::MEAN:
                BlurImageOps::mean(input, expected, radius_x, radius_y, storage);
                BlurImageOps_MT::mean(input, found, radius_x, radius_y, storage);
                break;
            case Op::MEAN_BORDER:
                BlurImageOps::mean_border(input, expected, rx, ry, *border, storage, totals);
                BlurImageOps_MT::mean_border(input, found, rx, ry, *border, storage);
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
                ConvolveImageMean::horizontal(input, expected, rx, 2*rx + 1, *border);
                ConvolveImageMean_MT::horizontal(input, found, rx, 2*rx + 1, *border);
                break;
            case Op::VERTICAL_IMAGE_BORDER:
                ConvolveImageMean::vertical(input, expected, ry, 2*ry + 1, *border, totals);
                ConvolveImageMean_MT::vertical(input, found, ry, 2*ry + 1, *border);
                break;
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " radius " +
                                         to_string(radius_x) + "," + to_string(radius_y));
    }

    /// Image sizes smaller and larger than the kernels, heights not a multiple of the blocks, and radius_x and
    /// radius_y that differ
    template<class E>
    void check_all() {
        for (Op op : {Op::MEAN, Op::MEAN_BORDER, Op::HORIZONTAL_NORMALIZED, Op::VERTICAL_NORMALIZED,
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
};

TEST_F(BlurImageOps_MTTest, U8) { check_all<U8>(); }
TEST_F(BlurImageOps_MTTest, U16) { check_all<U16>(); }
