#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/geometry_types.h"

using namespace std;
using namespace boofcv;

class Point2DTest : public boofcv::BoofCPPTest {};

TEST_F(Point2DTest, distanceSq_int) {
    Point2D<U8> a(10,20);
    Point2D<U8> b(15,22);

    ASSERT_EQ(5*5+4,a.distanceSq(b));
}

TEST_F(Point2DTest, distanceSq_float) {
    Point2D<F32> a(10,20);
    Point2D<F32> b(15,22);

    ASSERT_FLOAT_EQ(29.0f,a.distanceSq(b));
}