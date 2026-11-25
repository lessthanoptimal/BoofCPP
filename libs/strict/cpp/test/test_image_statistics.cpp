#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/base_types.h"
#include "boofcv/image_statistics.h"
#include "boofcv/image_misc_ops.h"

using namespace boofcv;
using namespace std;

class ImageStatisticsTest : public boofcv::BoofCPPTest {};

/// Calls check() on the image, then on a copy of it inside a larger image filled with 'outside', a value that
/// changes the result if it's read
template<class T, class Check>
void with_subimage( const Gray<T>& image, T outside, Check check ) {
    check(image);
    Gray<T> parent(image.width + 4, image.height + 6);
    ImageMiscOps::fill(parent, outside);
    Gray<T> sub = parent.makeSubimage(2, 3, 2 + image.width, 3 + image.height);
    sub.copy(image);
    check(sub);
}

TEST_F(ImageStatisticsTest, min) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)10);
    image.at(2,3) = 6;

    with_subimage(image, (U8)0, [](const Gray<U8>& img) { ASSERT_EQ(6,ImageStatistics::min(img)); });
}

TEST_F(ImageStatisticsTest, max) {
    Gray<F32> image(5,6);

    ImageMiscOps::fill(image,-2.0f);
    image.at(2,3) = -1.5f;

    with_subimage(image, 100.0f, [](const Gray<F32>& img) { ASSERT_EQ(-1.5f,ImageStatistics::max(img)); });
}

TEST_F(ImageStatisticsTest, min_max_NaN) {
    // NaN pixels after the first are skipped, a NaN first pixel is the result
    Gray<F32> image(4,3);
    ImageMiscOps::fill(image,2.0f);
    image.at(3,2) = 3.0f;
    image.at(1,1) = 1.0f;
    image.at(2,0) = NAN;
    ASSERT_EQ(3.0f,ImageStatistics::max(image));
    ASSERT_EQ(1.0f,ImageStatistics::min(image));
    image.at(0,0) = NAN;
    ASSERT_TRUE(std::isnan(ImageStatistics::max(image)));
    ASSERT_TRUE(std::isnan(ImageStatistics::min(image)));
}

TEST_F(ImageStatisticsTest, sum) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);

    with_subimage(image, (U8)7, [](const Gray<U8>& img) { ASSERT_EQ(2*5*6,ImageStatistics::sum(img)); });
}

TEST_F(ImageStatisticsTest, sum_rowOrder) {
    // Values of mixed magnitude, so the order of additions changes the float result
    Gray<F32> image(7,5);
    for( uint32_t y = 0; y < image.height; y++ )
        for( uint32_t x = 0; x < image.width; x++ )
            image.at(x,y) = (F32)(1.1 + x + 3*y)*(F32)pow(10,(x+2*y)%6);

    F32 expected = 0;
    for( uint32_t y = 0; y < image.height; y++ ) {
        F32 row_total = 0;
        for( uint32_t x = 0; x < image.width; x++ )
            row_total += image.at(x,y);
        expected += row_total;
    }
    ASSERT_EQ(expected,ImageStatistics::sum(image));
    ASSERT_EQ(expected/(F32)(7*5),ImageStatistics::mean(image));
}

TEST_F(ImageStatisticsTest, sum_exceeds32bits) {
    Gray<U16> u16(256,257);
    ImageMiscOps::fill(u16,(U16)65535);
    ASSERT_EQ((int64_t)65535*256*257,ImageStatistics::sum(u16));

    Gray<S32> s32(3,2);
    ImageMiscOps::fill(s32,numeric_limits<S32>::max());
    ASSERT_EQ((int64_t)numeric_limits<S32>::max()*6,ImageStatistics::sum(s32));
}

TEST_F(ImageStatisticsTest, mean) {
    Gray<U8> image(5,6);

    ImageMiscOps::fill(image,(U8)2);
    image.at(1,1) = 9;

    ASSERT_EQ(67.0/30.0,ImageStatistics::mean(image));
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
/// Pixels without a bin throw, including pixels below minValue when the histogram has more bins than
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

/// minValue outside the pixel type's range: a negative minValue shifts every bin, one above
/// every pixel value leaves them all without a bin
template<class T>
void check_histogram_unsigned_min_outside_type() {
    Gray<T> image(5,6);
    ImageMiscOps::fill(image,(T)2);
    image.at(1,1) = 0;
    GrowArray<uint32_t> histogram(8);
    ImageStatistics::histogram(image,-3,histogram);
    ASSERT_EQ(29u,histogram[5]);
    ASSERT_EQ(1u,histogram[3]);

    GrowArray<uint32_t> above(3);
    ASSERT_THROW(ImageStatistics::histogram(image,(int32_t)std::numeric_limits<T>::max() + 1,above), std::out_of_range);
}

TEST_F(ImageStatisticsTest, histogram_unsigned_min_outside_type) {
    check_histogram_unsigned_min_outside_type<U8>();
    check_histogram_unsigned_min_outside_type<U16>();
}

TEST_F(ImageStatisticsTest, histogram_U8_outside) {
    check_histogram_unsigned_outside<U8>();
}

TEST_F(ImageStatisticsTest, histogram_U16_outside) {
    check_histogram_unsigned_outside<U16>();
}

/// Bins are (int)(value - minValue). Values less than 1 outside the histogram and NaN are counted
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

/// U8 against a pixel by pixel count: widths that aren't a multiple of the 4 histogram copies, a sub-image, values
/// from a few to all 256, and minValue below, at and above the smallest pixel
TEST_F(ImageStatisticsTest, histogram_U8_counts) {
    for( uint32_t width : {1u, 3u, 4u, 7u, 37u} ) {
        for( auto values : {std::pair<int, int>{0, 255}, {20, 23}, {90, 90}} ) {
            Gray<U8> parent(width + 3, 9);
            ImageMiscOps::fill_uniform(parent, (U8)values.first, (U8)values.second, rng);
            Gray<U8> image = parent.makeSubimage(1, 2, 1 + width, 9);
            for( int32_t minValue : {-3, 0, values.first, values.first + 1} ) {
                GrowArray<uint32_t> histogram(300);
                std::vector<uint32_t> expected(300, 0);
                bool outside = false;
                for( uint32_t y = 0; y < image.height; y++ ) {
                    for( uint32_t x = 0; x < image.width; x++ ) {
                        const int32_t bin = image.at(x, y) - minValue;
                        if( bin < 0 )
                            outside = true;
                        else
                            expected[bin]++;
                    }
                }
                if( outside ) {
                    ASSERT_THROW(ImageStatistics::histogram(image, minValue, histogram), std::out_of_range);
                    continue;
                }
                ImageStatistics::histogram(image, minValue, histogram);
                for( uint32_t i = 0; i < 300; i++ )
                    ASSERT_EQ(expected[i], histogram[i]) << width << " " << minValue << " " << i;
            }
        }
    }
}
