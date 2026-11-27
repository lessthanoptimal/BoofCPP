#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/threshold_block_filters_mt.h"

#include <string>

using namespace std;
using namespace boofcv;

/// ThresholdBlock_MT must be identical to its filter for any number of threads
class ThresholdBlock_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Compares a filter made by create(local) with and without local blocks, on a sub-image
    template<class Filter, class E, class Create>
    void check_matches_single_threaded( uint32_t width, uint32_t height, Create create ) {
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)255, rng);
        Gray<U8> parent_expected, parent_found;
        Gray<U8> expected = subimage(parent_expected, width, height);
        Gray<U8> found = subimage(parent_found, width, height);
        for (bool local : {true, false}) {
            Filter single = create(local);
            ThresholdBlock_MT<Filter> concurrent(create(local));
            single.process(input, expected);
            concurrent.process(input, found);
            check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " local " +
                                             to_string(local));
        }
    }

    template<class Filter, class E, class Create>
    void check_all( Create create ) {
        for_each_setting([&] {
            for (auto size : {make_pair(4u, 4u), make_pair(13u, 40u), make_pair(40u, 13u), make_pair(41u, 23u)})
                check_matches_single_threaded<Filter, E>(size.first, size.second, create);
        });
        check_blocks_out_of_order([&]( uint32_t scale ) {
            check_matches_single_threaded<Filter, E>(64*scale, 32*scale, create);
        });
    }
};

TEST_F(ThresholdBlock_MTTest, mean) {
    auto create = []( bool local ) { return ThresholdBlockMean<U8>(ConfigLength::fixed(4), local, 0.95, true); };
    check_all<ThresholdBlockMean<U8>, U8>(create);
}

TEST_F(ThresholdBlock_MTTest, min_max) {
    auto create = []( bool local ) { return ThresholdBlockMinMax<U8>(1.0f, ConfigLength::fixed(4), local, 0.95, true); };
    check_all<ThresholdBlockMinMax<U8>, U8>(create);
}

TEST_F(ThresholdBlock_MTTest, otsu) {
    auto create = []( bool local ) { return ThresholdBlockOtsu<U8>(false, ConfigLength::fixed(4), 0.3, 0.95, true, local); };
    check_all<ThresholdBlockOtsu<U8>, U8>(create);
}
