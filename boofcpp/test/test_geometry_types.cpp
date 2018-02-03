#include "gtest/gtest.h"
#include "geometry_types.h"

using namespace std;
using namespace boofcv;

TEST(Point2D, distanceSq_int) {
    Point2D<U8> a(10,20);
    Point2D<U8> b(15,22);

    ASSERT_EQ(5*5+4,a.distanceSq(b));
}

TEST(Point2D, distanceSq_float) {
    Point2D<F32> a(10,20);
    Point2D<F32> b(15,22);

    ASSERT_FLOAT_EQ(29.0f,a.distanceSq(b));
}