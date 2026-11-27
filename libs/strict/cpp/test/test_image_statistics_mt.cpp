#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/image_statistics_mt.h"

using namespace std;
using namespace boofcv;

/// ImageStatistics_MT must be identical to ImageStatistics for any number of threads
class ImageStatistics_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Histograms of a sub-image with values in [low, high] and the histogram starting at 'min_value'
    template<class E, class V>
    void check_histogram( uint32_t width, uint32_t height, E low, E high, V min_value, uint32_t bins ) {
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, low, high, rng);
        GrowArray<uint32_t> expected(bins), found(bins);
        found.fill(7);
        ImageStatistics::histogram(input, min_value, expected);
        ImageStatistics_MT::histogram(input, min_value, found);
        for (uint32_t i = 0; i < bins; i++)
            ASSERT_EQ(expected[i], found[i]) << "bin " << i << " size " << width << "x" << height;
    }

    template<class E, class V>
    void check_all( E low, E high, V min_value, uint32_t bins ) {
        for_each_setting([&] {
            for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(41u, 23u)})
                check_histogram<E>(size.first, size.second, low, high, min_value, bins);
        });
        check_blocks_out_of_order([&]( uint32_t scale ) {
            check_histogram<E>(64*scale, 32*scale, low, high, min_value, bins);
        });
    }
};

TEST_F(ImageStatistics_MTTest, histogram_U8) { check_all<U8>((U8)20, (U8)200, (int32_t)10, 200u); }
TEST_F(ImageStatistics_MTTest, histogram_F32) { check_all<F32>(5.0f, 120.5f, 4.5f, 117u); }

