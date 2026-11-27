#include "gtest/gtest.h"
#include "testing_concurrency.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/stereo_disparity.h"
#include "boofcv/stereo_disparity_mt.h"

#include <string>

using namespace std;
using namespace boofcv;

/// FactoryStereoDisparity_MT must be identical to FactoryStereoDisparity for any number of threads
class FactoryStereoDisparity_MTTest : public boofcv::ConcurrentTest {
protected:
    /// Every error type, so every part BlockMatchParts_MT replaces is used: SAD and SSD only match blocks, ZSSD and
    /// NCC without normalization convert the input, NCC with it normalizes, and census transforms it
    template<class E>
    void check_every_error() {
        Gray<E> left(30, 20), right(30, 20);
        ImageMiscOps::fill_uniform(left, (E)0, (E)100, rng);
        ImageMiscOps::fill_uniform(right, (E)0, (E)100, rng);

        const pair<DisparityError, bool> cases[] = {
                {DisparityError::SAD, false}, {DisparityError::SSD, false}, {DisparityError::ZSSD, false},
                {DisparityError::NCC, false}, {DisparityError::NCC, true}, {DisparityError::CENSUS, false}};
        for (const auto& [error, normalize] : cases) {
            ConfigDisparityBM config;
            config.error_type = error;
            config.config_ncc.normalize_input = normalize;
            config.disparity_range = 10;
            config.save_score = true;
            const string what = string(DISPARITY_ERROR_NAMES[(int)error]) + " normalize " + to_string(normalize);

            auto expected = FactoryStereoDisparity::block_match<E, F32>(config);
            expected->process(left, right);
            for_each_setting([&] {
                auto found = FactoryStereoDisparity_MT::block_match<E, F32>(config);
                const uint64_t before = BoofConcurrency::testing_loops();
                found->process(left, right);
                EXPECT_LT(before, BoofConcurrency::testing_loops()) << what;
                check_identical(expected->get_disparity(), found->get_disparity(), what);
                check_identical(*expected->get_score(), *found->get_score(), what);
            });
        }
    }
};

TEST_F(FactoryStereoDisparity_MTTest, U8) { check_every_error<U8>(); }
TEST_F(FactoryStereoDisparity_MTTest, F32) { check_every_error<F32>(); }

class FactoryCensusTransform_MTTest : public boofcv::ConcurrentTest {};

/// Every variant runs the _MT code with more than one thread
TEST_F(FactoryCensusTransform_MTTest, variant_is_concurrent) {
    BoofConcurrency::set_max_threads(2);
    BoofConcurrency::set_testing_blocks(4);
    Gray<U8> input(30, 20);
    auto check = [&]( auto census, auto output ) {
        const uint64_t before = BoofConcurrency::testing_loops();
        census->process(input, output);
        EXPECT_LT(before, BoofConcurrency::testing_loops());
    };
    check(FactoryCensusTransform_MT::variant<U8, U8>(CensusVariants::BLOCK_3_3, REFLECT), Gray<U8>());
    check(FactoryCensusTransform_MT::variant<U8, S32>(CensusVariants::BLOCK_5_5, REFLECT), Gray<S32>());
    check(FactoryCensusTransform_MT::variant<U8, S64>(CensusVariants::CIRCLE_9, REFLECT), Gray<S64>());
}
