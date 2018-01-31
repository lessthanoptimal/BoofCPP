#include "gtest/gtest.h"
#include "config_types.h"

using namespace std;
using namespace boofcv;

TEST(ConfigLength, fixed) {
    ConfigLength c = ConfigLength::fixed(2.4);

    EXPECT_TRUE(c.isFixed());
    EXPECT_FALSE(c.isRelative());
    EXPECT_EQ(2.4, c.compute(123));
    EXPECT_EQ(2.4, c.compute(3));
}

TEST(ConfigLength, relative) {
    ConfigLength c = ConfigLength::relative(0.5,2);

    EXPECT_FALSE(c.isFixed());
    EXPECT_TRUE(c.isRelative());
    EXPECT_EQ(2.5, c.compute(5));
    EXPECT_EQ(2.0, c.compute(2.2));
}