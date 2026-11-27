#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/image_normalization_mt.h"

#include <cmath>
#include <limits>
#include <string>

using namespace std;
using namespace boofcv;

/// ImageNormalization_MT must be identical to ImageNormalization for any number of threads
class ImageNormalization_MTTest : public boofcv::ConcurrentTest {
protected:
    template<class E>
    void check_matches_single_threaded( uint32_t width, uint32_t height ) {
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        if constexpr (std::is_floating_point<E>::value)
            ImageMiscOps::fill_uniform(input, (E)-250, (E)300, rng);
        else
            ImageMiscOps::fill_uniform(input, (E)0, (E)255, rng);
        const string name = "size " + to_string(width) + "x" + to_string(height);

        Gray<F32> parent_expected, parent_found;
        Gray<F32> expected = subimage(parent_expected, width, height);
        Gray<F32> found = subimage(parent_found, width, height);
        NormalizeParameters p_expected, p_found;
        ImageNormalization::zero_mean_max_one(input, expected, p_expected);
        ImageNormalization_MT::zero_mean_max_one(input, found, p_found);
        check_identical(expected, found, name + " zero_mean_max_one");
        EXPECT_EQ(p_expected.offset, p_found.offset) << name;
        EXPECT_EQ(p_expected.divisor, p_found.divisor) << name;

        ImageNormalization::apply(input, p_expected, expected);
        ImageNormalization_MT::apply(input, p_expected, found);
        check_identical(expected, found, name + " apply");

        EXPECT_EQ(ImageNormalization::max_abs(input), ImageNormalization_MT::max_abs(input)) << name;
        EXPECT_EQ(ImageNormalization::mean_F32(expected), ImageNormalization_MT::mean_F32(expected)) << name;
    }

    template<class E>
    void check_all() {
        for_each_setting([&] {
            for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(41u, 23u)})
                check_matches_single_threaded<E>(size.first, size.second);
        });
        check_blocks_out_of_order([&]( uint32_t scale ) { check_matches_single_threaded<E>(64*scale, 32*scale); });
    }
};

TEST_F(ImageNormalization_MTTest, U8) { check_all<U8>(); }
TEST_F(ImageNormalization_MTTest, F32) { check_all<F32>(); }

/// A NaN is never the largest absolute value, unless it's the first pixel, the same as single threaded
TEST_F(ImageNormalization_MTTest, max_abs_NaN) {
    for_each_setting([&] {
        Gray<F32> image(41, 23);
        ImageMiscOps::fill_uniform(image, -10.0f, 10.0f, rng);
        image.unsafe_at(5, 17) = std::numeric_limits<F32>::quiet_NaN();
        EXPECT_EQ(ImageNormalization::max_abs(image), ImageNormalization_MT::max_abs(image));
        image.unsafe_at(0, 0) = std::numeric_limits<F32>::quiet_NaN();
        EXPECT_TRUE(std::isnan(ImageNormalization::max_abs(image)));
        EXPECT_TRUE(std::isnan(ImageNormalization_MT::max_abs(image)));
    });
}
