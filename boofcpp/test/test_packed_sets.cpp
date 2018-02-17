#include "gtest/gtest.h"
#include "packed_sets.h"

using namespace std;
using namespace boofcv;

class TD {
public:
    uint32_t x = 0;
    uint32_t y = 0;

    TD(uint32_t x, uint32_t y ){
        this->x = x;
        this->y = y;
    }

    TD() = default;
};

TEST(PackedSet, constructor) {
    PackedSet<TD> packed(100);

    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(0,packed._total_element);
    ASSERT_EQ(0,packed.set_info.size());
}

TEST(PackedSet, _add_element) {
    PackedSet<TD> packed(100);

    packed._add_element(TD(5,2));

    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(1,packed._total_element);
    ASSERT_EQ(5,packed.blocks[0][0].x);
    ASSERT_EQ(2,packed.blocks[0][0].y);
    ASSERT_EQ(0,packed.set_info.size());

    packed._add_element(TD(6,7));
    ASSERT_EQ(2,packed._total_element);
    ASSERT_EQ(6,packed.blocks[0][1].x);
    ASSERT_EQ(7,packed.blocks[0][1].y);
}

TEST(PackedSet, _add_element_setoverflow) {
    PackedSet<TD> packed(10);

    for (uint32_t i = 0; i < 25; i++) {
        packed._add_element(TD(5+i, 2));
    }

    ASSERT_EQ(3,packed._number_of_blocks);
    ASSERT_EQ(10,packed._size_of_block);
    ASSERT_EQ(25,packed._total_element);
    ASSERT_EQ(0,packed.set_info.size());

    for (uint32_t i = 0; i < 25; i++) {
        TD& d = packed.blocks[i/10][i%10];
        ASSERT_EQ(5+i,d.x);
        ASSERT_EQ(2,d.y);
    }
}

TEST(PackedSet, push_tail) {
    PackedSet<TD> packed(100);

    ASSERT_EQ(0,packed.set_info.size());
    packed.push_tail(TD(5,2));
    ASSERT_EQ(1,packed.set_info.size());
    packed.push_tail(TD(6,7));

    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(2,packed._total_element);
    ASSERT_EQ(5,packed.blocks[0][0].x);
    ASSERT_EQ(2,packed.blocks[0][0].y);
    ASSERT_EQ(6,packed.blocks[0][1].x);
    ASSERT_EQ(7,packed.blocks[0][1].y);
    ASSERT_EQ(1,packed.set_info.size());
    ASSERT_EQ(2,packed.set_info.at(0).size);
}


TEST(PackedSet, size_of_tail) {
    PackedSet<TD> packed(100);

    ASSERT_EQ(0,packed.size_of_tail());
    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));
    ASSERT_EQ(2,packed.size_of_tail());
}

TEST(PackedSet, start_new_set) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(3,packed._total_element);
    ASSERT_EQ(5,packed.blocks[0][0].x);
    ASSERT_EQ(2,packed.blocks[0][0].y);
    ASSERT_EQ(6,packed.blocks[0][1].x);
    ASSERT_EQ(7,packed.blocks[0][1].y);
    ASSERT_EQ(6,packed.blocks[0][2].x);
    ASSERT_EQ(5,packed.blocks[0][2].y);
    ASSERT_EQ(2,packed.set_info.size());
    ASSERT_EQ(0,packed.set_info.at(0).offset);
    ASSERT_EQ(2,packed.set_info.at(0).size);
    ASSERT_EQ(2,packed.set_info.at(1).offset);
    ASSERT_EQ(1,packed.set_info.at(1).size);
}

TEST(PackedSet, reset) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    packed.clear();
    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(0,packed._total_element);
    ASSERT_EQ(0,packed.set_info.size());
}

TEST(PackedSet, at) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    ASSERT_EQ(5,packed.at(0,0).x);
    ASSERT_EQ(2,packed.at(0,0).y);
    ASSERT_EQ(6,packed.at(0,1).x);
    ASSERT_EQ(7,packed.at(0,1).y);
    ASSERT_EQ(6,packed.at(1,0).x);
    ASSERT_EQ(5,packed.at(1,0).y);
}

TEST(PackedSet, remove_tail_set) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    ASSERT_EQ(1,packed._number_of_blocks);
    ASSERT_EQ(100,packed._size_of_block);
    ASSERT_EQ(3,packed._total_element);
    ASSERT_EQ(2,packed.set_info.size());
    ASSERT_EQ(2,packed.set_info.at(0).size);
    ASSERT_EQ(1,packed.set_info.at(1).size);

    packed.remove_tail();
    ASSERT_EQ(2,packed._total_element);
    ASSERT_EQ(1,packed.set_info.size());

    packed.remove_tail();
    ASSERT_EQ(0,packed._total_element);
    ASSERT_EQ(0,packed.set_info.size());

    packed.remove_tail();
    ASSERT_EQ(0,packed._total_element);
    ASSERT_EQ(0,packed.set_info.size());
}

TEST(PackedSet, load_set) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    std::vector<TD> storage;

    packed.load_set(0,storage);
    ASSERT_EQ(2,storage.size());
    ASSERT_EQ(5,storage.at(0).x);
    ASSERT_EQ(2,storage.at(0).y);
    ASSERT_EQ(6,storage.at(1).x);
    ASSERT_EQ(7,storage.at(1).y);

    storage.clear();
    packed.load_set(1,storage);
    ASSERT_EQ(1,storage.size());
    ASSERT_EQ(6,storage.at(0).x);
    ASSERT_EQ(5,storage.at(0).y);
}

TEST(PackedSet, write_set) {
    PackedSet<TD> packed(100);

    packed.push_tail(TD(5,2));
    packed.push_tail(TD(6,7));

    packed.start_new_set();
    packed.push_tail(TD(6,5));

    std::vector<TD> storage;
    storage.push_back(TD(1,2));
    storage.push_back(TD(3,4));

    packed.write_set(0,storage);

    ASSERT_EQ(1,packed.at(0,0).x);
    ASSERT_EQ(2,packed.at(0,0).y);
    ASSERT_EQ(3,packed.at(0,1).x);
    ASSERT_EQ(4,packed.at(0,1).y);
}