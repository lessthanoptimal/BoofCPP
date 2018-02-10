#include "gtest/gtest.h"
#include "packed_sets.h"

using namespace std;
using namespace boofcv;

struct TD {
    uint32_t x = 0;
    uint32_t y = 0;
};

TEST(PackedSet, constructor) {
    PackedSet<TD> packed;
}

TEST(PackedSet, _add_element) {
    PackedSet<TD> packed;
}

TEST(PackedSet, push_into_tail) {
    PackedSet<TD> packed;
}