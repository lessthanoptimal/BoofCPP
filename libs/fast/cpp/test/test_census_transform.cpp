#include "gtest/gtest.h"
#include "testing_utils.h"

// Fast's census supports U8, U16 and F32
#include "variant_fast.h"
#include "shared/census_checks.h"

using namespace boofcv;

class CensusTransformTest : public boofcv::BoofCPPTest {};

TEST_F(CensusTransformTest, U8) { census_checks::check_all<U8>(); }
TEST_F(CensusTransformTest, U16) { census_checks::check_all<U16>(); }
TEST_F(CensusTransformTest, F32) { census_checks::check_all<F32>(); }
TEST_F(CensusTransformTest, too_many_samples) { census_checks::check_too_many_samples<U8>(); }
