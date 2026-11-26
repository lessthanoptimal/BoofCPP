#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/pixel_math_mt.h"

#include <string>

using namespace std;
using namespace boofcv;

/// PixelMath_MT and ConvertImage_MT must be identical to PixelMath and convert() for any number of threads
class PixelMath_MTTest : public boofcv::ConcurrentTest {
protected:
    enum class Op { POW2, STDEV, PLUS, MINUS, MULTIPLY, CONVERT };

    /// Compares one operation with the current settings, on sub-image inputs and outputs
    void check_matches_single_threaded( Op op, uint32_t width, uint32_t height ) {
        Gray<U8> parent_u8;
        Gray<U8> input_u8 = subimage(parent_u8, width, height);
        ImageMiscOps::fill_uniform(input_u8, (U8)0, (U8)255, rng);
        Gray<F32> parent_a, parent_b, parent_expected, parent_found;
        Gray<F32> a = subimage(parent_a, width, height);
        Gray<F32> b = subimage(parent_b, width, height);
        ImageMiscOps::fill_uniform(a, -100.0f, 100.0f, rng);
        ImageMiscOps::fill_uniform(b, 0.0f, 10000.0f, rng);
        Gray<F32> expected = subimage(parent_expected, width, height);
        Gray<F32> found = subimage(parent_found, width, height);
        switch (op) {
            case Op::POW2:
                PixelMath::pow2(a, expected);
                PixelMath_MT::pow2(a, found);
                break;
            case Op::STDEV:
                PixelMath::stdev(a, b, expected);
                PixelMath_MT::stdev(a, b, found);
                break;
            case Op::PLUS:
                PixelMath::plus(input_u8, 1.7f, expected);
                PixelMath_MT::plus(input_u8, 1.7f, found);
                break;
            case Op::MINUS:
                PixelMath::minus(a, 1.7f, expected);
                PixelMath_MT::minus(a, 1.7f, found);
                break;
            case Op::MULTIPLY:
                PixelMath::multiply(input_u8, 0.37f, expected);
                PixelMath_MT::multiply(input_u8, 0.37f, found);
                break;
            case Op::CONVERT:
                convert(input_u8, expected);
                ConvertImage_MT::convert(input_u8, found);
                break;
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height));
    }
};

TEST_F(PixelMath_MTTest, all) {
    for (Op op : {Op::POW2, Op::STDEV, Op::PLUS, Op::MINUS, Op::MULTIPLY, Op::CONVERT}) {
        for_each_setting([&] {
            for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(41u, 23u)})
                check_matches_single_threaded(op, size.first, size.second);
        });
        check_shuffled([&]( uint32_t scale ) { check_matches_single_threaded(op, 64*scale, 32*scale); });
    }
}

/// The output can be the input, as ImageNormalization and the disparity statistics use it
TEST_F(PixelMath_MTTest, in_place) {
    for_each_setting([&] {
        Gray<F32> expected(41, 23), found(41, 23);
        ImageMiscOps::fill_uniform(expected, -100.0f, 100.0f, rng);
        found.copy(expected);
        PixelMath::minus(expected, 3.5f, expected);
        PixelMath_MT::minus(found, 3.5f, found);
        check_identical(expected, found, "minus");
        Gray<F32> mean(41, 23);
        ImageMiscOps::fill_uniform(mean, -10.0f, 10.0f, rng);
        ImageMiscOps::fill_uniform(expected, 0.0f, 1000.0f, rng);
        found.copy(expected);
        PixelMath::stdev(mean, expected, expected);
        PixelMath_MT::stdev(mean, found, found);
        check_identical(expected, found, "stdev");
    });
}
