#include "gtest/gtest.h"
#include "testing_utils.h"

#include "boofcv/binary_ops.h"
#include "variant_fast.h"
#include "shared/threshold_checks.h"

using namespace boofcv;

class GlobalFixedBinaryFilterTest : public boofcv::BoofCPPTest {};
TEST_F(GlobalFixedBinaryFilterTest, double_threshold) { threshold_checks::global_fixed_double_threshold(); }
