#include "gtest/gtest.h"
#include "testing_utils.h"

// Strict runs every check
#include "variant_strict.h"
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
