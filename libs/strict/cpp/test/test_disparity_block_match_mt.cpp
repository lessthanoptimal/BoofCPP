#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/disparity_block_match_mt.h"
#include "boofcv/image_misc_ops.h"

#include <memory>
#include <string>

using namespace std;
using namespace boofcv;

/// DisparityBlockMatchRowFormat_MT must be identical to DisparityBlockMatchRowFormat for any number of threads
class DisparityBlockMatchRowFormat_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Runs both on random images, each with its own selector from create_select(), and compares the disparity and
    /// score images. radius_x and radius_y differ, the smallest disparity is negative, and a catastrophic_reset of 4
    /// splits floating point scores into several chunks
    template<class E, class S, class D, class CreateSelect>
    void check_matches_single_threaded( uint32_t width, uint32_t height, shared_ptr<BlockRowScore<E, S>> row_score,
                                        CreateSelect create_select ) {
        Gray<E> left(width, height), right(width, height);
        ImageMiscOps::fill_uniform(left, (E)0, (E)100, rng);
        ImageMiscOps::fill_uniform(right, (E)0, (E)100, rng);

        auto run = [&]( DisparityBlockMatchRowFormat<E, S>& alg, DisparityOutput<D>& select, Gray<D>& disparity,
                        Gray<F32>& score ) {
            alg.configure(-2, 9);
            alg.set_border(REFLECT);
            alg.set_catastrophic_reset(4);
            ImageMiscOps::fill(disparity, (D)1);
            ImageMiscOps::fill(score, -2.0f);
            select.set_output(disparity, &score);
            alg.process(left, right);
        };
        Gray<D> expected(width, height), found(width, height);
        Gray<F32> expected_score(width, height), found_score(width, height);
        auto select_single = create_select();
        DisparityBlockMatchRowFormat<E, S> single(2, 3, row_score, select_single);
        run(single, *select_single, expected, expected_score);
        auto select_mt = create_select();
        DisparityBlockMatchRowFormat_MT<E, S> mt(2, 3, row_score, select_mt);
        run(mt, *select_mt, found, found_score);

        const string what = "size " + to_string(width) + "x" + to_string(height);
        check_identical(expected, found, what + " disparity");
        check_identical(expected_score, found_score, what + " score");
    }

    /// Fewer rows than the region and several blocks, then blocks that finish out of order
    template<class E, class S, class D, class CreateRowScore, class CreateSelect>
    void check_all( CreateRowScore create_row_score, CreateSelect create_select ) {
        for_each_setting([&] {
            for (uint32_t height : {2u, 23u})
                check_matches_single_threaded<E, S, D>(30, height, create_row_score(), create_select);
        });
        check_shuffled([&]( uint32_t scale ) {
            check_matches_single_threaded<E, S, D>(30, 32*scale, create_row_score(), create_select);
        });
    }
};

/// Integer scores, with every check, so the selector's work space is used
TEST_F(DisparityBlockMatchRowFormat_MTTest, SAD_U8) {
    check_all<U8, int32_t, U8>([] { return make_shared<BlockRowScoreSad<U8>>(); },
                               [] { return make_shared<SelectErrorWithChecks<int32_t, U8>>(2000, 1, 0.1); });
}

/// Floating point scores, which are split on catastrophic_reset, and subpixel disparity
TEST_F(DisparityBlockMatchRowFormat_MTTest, SAD_F32) {
    check_all<F32, F32, F32>([] { return make_shared<BlockRowScoreSad<F32>>(); },
                             [] { return make_shared<SelectErrorWithChecks<F32, F32>>(2000, 1, 0.1); });
}

/// Scores that are normalized before the selector
TEST_F(DisparityBlockMatchRowFormat_MTTest, NCC) {
    check_all<F32, F32, U8>([] { return make_shared<BlockRowScoreNcc>(2, 3, REFLECT, 1e-4f); },
                            [] { return make_shared<SelectCorrelationWithChecks_F32<U8>>(1, 0.1); });
}
