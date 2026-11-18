#include "gtest/gtest.h"
#include "testing_utils.h"

#include <random>
#include <vector>

// Fast runs every check
#include "variant_fast.h"
#include "shared/disparity_block_match_checks.h"

using namespace disparity_block_match_checks;

class GrowRowTest : public boofcv::BoofCPPTest {};
class BlockRowScoreSadTest : public boofcv::BoofCPPTest {};
class BlockRowScoreCensusTest : public boofcv::BoofCPPTest {};
class DisparityBlockMatchRowFormatTest : public boofcv::BoofCPPTest {};
class SelectErrorWithChecks_S32Test : public boofcv::BoofCPPTest {};
class BlockRowScoreSsdTest : public boofcv::BoofCPPTest {};
class BlockRowScoreZssdTest : public boofcv::BoofCPPTest {};
class BlockRowScoreNccTest : public boofcv::BoofCPPTest {};
class SelectCorrelationWithChecks_F32Test : public boofcv::BoofCPPTest {};

TEST_F(GrowRowTest, compare_to_border) { grow_row_compare_to_border(); }
TEST_F(BlockRowScoreSadTest, compare_to_naive) { block_row_score_sad_compare_to_naive(); }
TEST_F(BlockRowScoreCensusTest, compare_to_naive) { block_row_score_census_compare_to_naive(); }
TEST_F(DisparityBlockMatchRowFormatTest, compare_to_naive_U8) {
    disparity_block_match_row_format_compare_to_naive_U8();
}
TEST_F(DisparityBlockMatchRowFormatTest, compare_to_naive_subpixel) {
    disparity_block_match_row_format_compare_to_naive_subpixel();
}
TEST_F(DisparityBlockMatchRowFormatTest, run_twice_same_result) {
    disparity_block_match_row_format_run_twice_same_result();
}
TEST_F(SelectErrorWithChecks_S32Test, range_too_large_for_U8) { select_error_with_checks_s32_range_too_large_for_U8(); }
TEST_F(DisparityBlockMatchRowFormatTest, bad_input) { disparity_block_match_row_format_bad_input(); }
TEST_F(BlockRowScoreSadTest, compare_to_naive_F32) { block_row_score_sad_compare_to_naive_F32(); }
TEST_F(BlockRowScoreSsdTest, compare_to_naive) { block_row_score_ssd_compare_to_naive(); }
TEST_F(BlockRowScoreZssdTest, block_match_compare_to_naive) { block_row_score_zssd_block_match_compare_to_naive(); }
TEST_F(BlockRowScoreNccTest, block_match_compare_to_naive) { block_row_score_ncc_block_match_compare_to_naive(); }
TEST_F(DisparityBlockMatchRowFormatTest, F32_same_as_U8) { disparity_block_match_row_format_F32_same_as_U8(); }
TEST_F(DisparityBlockMatchRowFormatTest, catastrophic_reset_must_be_positive) {
    disparity_block_match_row_format_catastrophic_reset_must_be_positive();
}
TEST_F(SelectCorrelationWithChecks_F32Test, compare_to_naive) { select_correlation_with_checks_f32_compare_to_naive(); }
TEST_F(SelectCorrelationWithChecks_F32Test, range_too_large_for_U8) {
    select_correlation_with_checks_f32_range_too_large_for_U8();
}

namespace {
    /// Fast's selector with score type S against strict's int32_t selector, on random scores of one row
    template<class S, class D>
    void select_same_as_strict( int32_t width, int32_t dmin, int32_t range, double max_error, int32_t r2l,
                                double texture, int32_t low, int32_t high, uint32_t seed ) {
        std::mt19937 gen(seed);
        std::uniform_int_distribution<int32_t> dist(low, high);
        std::vector<int32_t> scores32(width*range);
        std::vector<S> scores(width*range);
        for (size_t i = 0; i < scores.size(); i++) {
            scores32[i] = dist(gen);
            scores[i] = (S)scores32[i];
        }
        boofcv::SelectErrorWithChecks<int32_t, D> expected(max_error, r2l, texture);
        boofcv::fast::SelectErrorWithChecks<S, D> found(max_error, r2l, texture);
        Gray<D> disparity_e(width, 2), disparity_f(width, 2);
        Gray<F32> score_e(width, 2), score_f(width, 2);
        expected.set_output(disparity_e, &score_e);
        found.set_output(disparity_f, &score_f);
        expected.configure(dmin, dmin + range - 1, width);
        found.configure(dmin, dmin + range - 1, width);
        expected.process(1, scores32.data());
        found.process(1, scores.data());
        for (int32_t x = 0; x < width; x++) {
            ASSERT_EQ(disparity_e.at(x, 1), disparity_f.at(x, 1)) << x << " range=" << range << " seed=" << seed;
            ASSERT_EQ(score_e.at(x, 1), score_f.at(x, 1)) << x << " range=" << range << " seed=" << seed;
        }
    }
}

/// 16-bit scores, including large ones where "no second best" must be int32_t's maximum like strict, and a few
/// values so there are many ties
TEST_F(SelectErrorWithChecks_S32Test, uint16_same_as_strict) {
    for (uint32_t seed = 0; seed < 20; seed++) {
        // width, dmin, range, max error, r2l, texture, low, high
        select_same_as_strict<uint16_t, U8>(30, 0, 3, 0, -1, 0.15, 60000, 65534, seed);
        select_same_as_strict<uint16_t, F32>(30, 0, 3, 0, 1, 0.15, 60000, 65534, seed);
        select_same_as_strict<uint16_t, U8>(30, -5, 12, 0, 1, 0.15, 0, 3, seed);
        select_same_as_strict<uint16_t, F32>(31, -3, 15, 30000, 0, 0.3, 0, 65534, seed);
        select_same_as_strict<uint16_t, F32>(12, 0, 20, 0, 1, 0.15, 0, 65534, seed);
        select_same_as_strict<int32_t, F32>(31, 2, 9, 0, 1, 0.15, 0, 1000000, seed);
        select_same_as_strict<int32_t, U8>(30, 0, 3, 0, -1, 0.15, 0, 2, seed);
    }
}
