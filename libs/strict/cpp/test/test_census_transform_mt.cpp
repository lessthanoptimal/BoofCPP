#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/census_transform_mt.h"
#include "boofcv/image_misc_ops.h"

#include <memory>
#include <string>

using namespace std;
using namespace boofcv;

/// CensusTransform_MT must be identical to CensusTransform for any number of threads
class CensusTransform_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Compares the operation of census type C (U8 dense3x3, S32 dense5x5, S64 sample_S64) with the current
    /// settings, on sub-image inputs and outputs. Without a border, the border pixels of the output must be left alone
    template<class E, class C>
    void check_matches_single_threaded( uint32_t width, uint32_t height, bool with_border ) {
        Gray<E> parent;
        Gray<E> input = subimage(parent, width, height);
        ImageMiscOps::fill_uniform(input, (E)0, (E)100, rng);
        Gray<C> expected_parent, found_out_parent;
        Gray<C> expected = subimage(expected_parent, width, height);
        Gray<C> found = subimage(found_out_parent, width, height);
        ImageMiscOps::fill(expected, (C)13);
        ImageMiscOps::fill(found, (C)13);
        shared_ptr<ImageBorder<E>> border = with_border ? FactoryImageBorder::create_SB<E>(REFLECT) : nullptr;
        // radius 4, the largest of the census variants
        const auto samples = CensusTransform::create_circle_samples();
        if constexpr (is_same<C, U8>::value) {
            CensusTransform::dense3x3(input, expected, border.get());
            CensusTransform_MT::dense3x3(input, found, border.get());
        } else if constexpr (is_same<C, S32>::value) {
            CensusTransform::dense5x5(input, expected, border.get());
            CensusTransform_MT::dense5x5(input, found, border.get());
        } else {
            CensusTransform::sample_S64(input, samples, expected, border.get());
            CensusTransform_MT::sample_S64(input, samples, found, border.get());
        }
        check_identical(expected, found, "size " + to_string(width) + "x" + to_string(height) + " border " +
                                         to_string(with_border));
    }

    /// Images too small for an inner image, and an image with several blocks. Then blocks that finish out of order
    template<class E, class C>
    void check_all() {
        for (bool with_border : {false, true}) {
            for_each_setting([&] {
                for (auto size : {make_pair(1u, 1u), make_pair(3u, 30u), make_pair(30u, 3u), make_pair(41u, 23u)})
                    check_matches_single_threaded<E, C>(size.first, size.second, with_border);
            });
            check_blocks_out_of_order([&]( uint32_t scale ) {
                check_matches_single_threaded<E, C>(64*scale, 32*scale, with_border);
            });
        }
    }
};

TEST_F(CensusTransform_MTTest, dense3x3) {
    check_all<U8, U8>();
    check_all<F32, U8>();
}

/// F32 has its own inner image code
TEST_F(CensusTransform_MTTest, dense5x5) {
    check_all<U8, S32>();
    check_all<F32, S32>();
}

TEST_F(CensusTransform_MTTest, sample_S64) {
    check_all<U8, S64>();
    check_all<F32, S64>();
}
