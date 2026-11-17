#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/base_types.h"
#include "boofcv/image_statistics.h"
#include "boofcv/image_misc_ops.h"

using namespace boofcv;
using namespace std;

class ImageStatisticsTest : public boofcv::BoofCPPTest {};

TEST_F(ImageStatisticsTest, min) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 6;

    ASSERT_EQ(6,ImageStatistics::min(image));
}

TEST_F(ImageStatisticsTest, max) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 12;

    ASSERT_EQ(12,ImageStatistics::max(image));
}

TEST_F(ImageStatisticsTest, sum) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);

    ASSERT_EQ(2*5*6,ImageStatistics::sum(image));
}

TEST_F(ImageStatisticsTest, mean) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);

    ASSERT_EQ(2,ImageStatistics::mean(image));
}

TEST_F(ImageStatisticsTest, histogram_INTEGER) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 12;
    image.at(2,4) = 12;
    image.at(2,5) = 12;
    image.at(0,0) = 6;

    GrowArray<uint32_t> histogram(256);

    ImageStatistics::histogram(image,(U8)0,histogram);

    for( int i = 0; i < 256; i++ ) {
        if( i == 6 ) {
            ASSERT_EQ(1,histogram[i]);
        } else if( i == 12 ) {
            ASSERT_EQ(3,histogram[i]);
        } else if( i == 10 ) {
            ASSERT_EQ(5*6-4,histogram[i]);
        } else {
            ASSERT_EQ(0,histogram[i]);
        }
    }
}

TEST_F(ImageStatisticsTest, histogram_FLOAT) {
    Gray<F32> image(5,6);

    ImageMiscOps::fill(image,(F32)10);
    image.at(2,3) = 12.1f;
    image.at(2,4) = 12.5f;
    image.at(2,5) = 12.9f;
    image.at(0,0) = 6.4f;

    GrowArray<uint32_t> histogram(256);

    ImageStatistics::histogram(image,(F32)0,histogram);

    for( int i = 0; i < 256; i++ ) {
        if( i == 6 ) {
            ASSERT_EQ(1,histogram[i]);
        } else if( i == 12 ) {
            ASSERT_EQ(3,histogram[i]);
        } else if( i == 10 ) {
            ASSERT_EQ(5*6-4,histogram[i]);
        } else {
            ASSERT_EQ(0,histogram[i]);
        }
    }
}
/// Pixels without a bin throw, same as BoofCV, including pixels below minValue when the histogram has more bins than
/// the type has values
template<class T>
void check_histogram_unsigned_outside() {
    Gray<T> image(5,6);
    GrowArray<uint32_t> histogram(5);

    ImageMiscOps::fill(image,(T)14);
    ImageStatistics::histogram(image,(T)10,histogram);
    ASSERT_EQ(30u,histogram[4]);

    for( T value : {(T)9, (T)15, (T)0, std::numeric_limits<T>::max()} ) {
        image.at(3,2) = value;
        ASSERT_THROW(ImageStatistics::histogram(image,(T)10,histogram), std::out_of_range);
    }

    GrowArray<uint32_t> large(300);
    image.at(3,2) = 9;
    ASSERT_THROW(ImageStatistics::histogram(image,(T)10,large), std::out_of_range);
}

TEST_F(ImageStatisticsTest, histogram_U8_outside) {
    check_histogram_unsigned_outside<U8>();
}

TEST_F(ImageStatisticsTest, histogram_U16_outside) {
    check_histogram_unsigned_outside<U16>();
}

/// Bins are (int)(value - minValue), same as BoofCV. Values less than 1 outside the histogram and NaN are counted
template<class T>
void check_histogram_float_outside() {
    Gray<T> image(5,6);
    GrowArray<uint32_t> histogram(5);

    ImageMiscOps::fill(image,(T)12);
    image.at(0,0) = (T)9.01;
    image.at(1,0) = (T)14.99;
    image.at(2,0) = std::numeric_limits<T>::quiet_NaN();
    ImageStatistics::histogram(image,(T)10,histogram);
    ASSERT_EQ(2u,histogram[0]);
    ASSERT_EQ(27u,histogram[2]);
    ASSERT_EQ(1u,histogram[4]);

    const T inf = std::numeric_limits<T>::infinity();
    for( T value : {(T)9, (T)15, inf, -inf, (T)1e30} ) {
        image.at(2,0) = value;
        ASSERT_THROW(ImageStatistics::histogram(image,(T)10,histogram), std::out_of_range);
    }
}

TEST_F(ImageStatisticsTest, histogram_F32_outside) {
    check_histogram_float_outside<F32>();
}

TEST_F(ImageStatisticsTest, histogram_F64_outside) {
    check_histogram_float_outside<F64>();
}
