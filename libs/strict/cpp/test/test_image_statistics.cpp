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

/// minValue outside the pixel type's range, as BoofCV's int allows: a negative minValue shifts every bin, one above
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
