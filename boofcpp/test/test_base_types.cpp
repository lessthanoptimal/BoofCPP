#include "gtest/gtest.h"
#include "base_types.h"

using namespace std;
using namespace boofcv;

TEST(IndependentMethod, GrowArray_initial_zero) {
    GrowArray<U8> array(20);

    for( uint32_t i = 0; i < 20; i++ ) {
        ASSERT_EQ(0,array.data[i]);
    }
}


TEST(IndependentMethod, GrowArray_at) {
    GrowArray<U8> array(5);

    ASSERT_EQ(0,array.at(1));
    array.at(1) = 5;
    ASSERT_EQ(5,array.data[1]);
    ASSERT_EQ(5,array.at(1));

    for( uint32_t i = 0; i < 5; i++ ) {
        if( i == 1 )
            continue;
        ASSERT_EQ(0,array.at(i));
    }
}

TEST(IndependentMethod, GrowArray_resize) {
    uint32_t N = 10;
    GrowArray<U8> array(N);

    for( uint32_t i = 0; i < N; i++ ){
        array[i] = i+1;
    }

    // resize so that it's smaller
    array.resize(N-2);
    ASSERT_EQ(N-2,array.size);
    ASSERT_EQ(N,array.array_length);
    for( uint32_t i = 0; i < N-2; i++ ){
        ASSERT_EQ(i+1,array[i]);
    }

    // resize so that it's bigger
    array.resize(N+2);
    ASSERT_EQ(N+2,array.size);
    ASSERT_EQ(N+2,array.array_length);
    for( uint32_t i = 0; i < N-2; i++ ){
        ASSERT_EQ(i+1,array[i]);
    }
    for( uint32_t i = N-2; i < N+2; i++ ){
        ASSERT_EQ(0,array[i]);
    }
}

TEST(IndependentMethod, GrowArray_grow) {
    uint32_t N = 10;
    GrowArray<U8> array(N);

    for( uint32_t i = 0; i < N; i++ ){
        array[i] = i+1;
    }

    // try growing to a smaller size. Nothing should happen
    array.grow_array(N-2);
    ASSERT_EQ(N,array.size);
    ASSERT_EQ(N,array.array_length);
    for( uint32_t i = 0; i < N; i++ ){
        ASSERT_EQ(i+1,array[i]);
    }

    //  Grow to a bigger size
    array.grow_array(N+2);
    ASSERT_EQ(N+2,array.size);
    ASSERT_EQ(N+2,array.array_length);
    for( uint32_t i = 0; i < N+2; i++ ){
        ASSERT_EQ(0,array[i]);
    }
}
