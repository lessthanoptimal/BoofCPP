#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/binary_ops_mt.h"
#include "boofcv/image_misc_ops.h"

#include <string>

using namespace std;
using namespace boofcv;

/// ThresholdOps_MT must be identical to ThresholdOps for any number of threads. U8 has its own local mean code
class ThresholdOps_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Every function on a sub-image, and local mean with regions smaller and larger than the image
    template<class E>
    void check_matches_single_threaded( uint32_t width, uint32_t height ) {
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)10, (E)200, rng);
        const string name = "size " + to_string(width) + "x" + to_string(height);
        Gray<U8> parent_expected, parent_found;
        Gray<U8> expected = subimage(parent_expected, width, height);
        Gray<U8> found = subimage(parent_found, width, height);

        ThresholdOps::threshold(input, (E)100, true, expected);
        ThresholdOps_MT::threshold(input, (E)100, true, found);
        check_identical(expected, found, name + " threshold");
        ThresholdOps::reshape_then_fill(input, (U8)1, expected);
        ThresholdOps_MT::reshape_then_fill(input, (U8)1, found);
        check_identical(expected, found, name + " reshape_then_fill");
        // One threshold: ThresholdOps::threshold_double() is shared, and its clamping is tested single threaded
        ImageMiscOps::fill(found, (U8)7);
        ThresholdOps::threshold_double(input, 99.5, true, expected);
        ThresholdOps_MT::threshold_double(input, 99.5, true, found);
        check_identical(expected, found, name + " threshold_double");
        Gray<E> storage1, storage2;
        for (uint32_t region : {3u, 61u}) {
            ThresholdOps::localMean(input, expected, ConfigLength::fixed(region), 0.95f, true, storage1, storage2);
            ThresholdOps_MT::localMean(input, found, ConfigLength::fixed(region), 0.95f, true, storage1, storage2);
            check_identical(expected, found, name + " localMean " + to_string(region));
        }
        EXPECT_EQ(ThresholdOps::computeOtsu(input, 5, 210), ThresholdOps_MT::computeOtsu(input, 5, 210)) << name;
    }

    template<class E>
    void check_all() {
        for_each_setting([&] {
            for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(41u, 23u)})
                check_matches_single_threaded<E>(size.first, size.second);
        });
        check_shuffled([&]( uint32_t scale ) { check_matches_single_threaded<E>(64*scale, 32*scale); });
    }
};

TEST_F(ThresholdOps_MTTest, U8) { check_all<U8>(); }
TEST_F(ThresholdOps_MTTest, F32) { check_all<F32>(); }

