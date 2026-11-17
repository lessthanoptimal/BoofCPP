#ifndef BOOFCPP_TESTING_UTILS_H
#define BOOFCPP_TESTING_UTILS_H

#include "gtest/gtest.h"
#include "boofcv/base_types.h"
#include "boofcv/image_types.h"
#include "boofcv/image_misc_ops.h"

#include <algorithm>
#include <random>
#include <type_traits>

namespace boofcv {

    /// Every gtest test uses a fixture derived from this, e.g. TEST_F(BlurImageOpsTest, mean), which
    /// test_code_standards checks
    class BoofCPPTest : public ::testing::Test {
    protected:
        std::mt19937 rng{23423};
    };

    /**
     * Returns a subimage which is equivalent to the input image. There will be a memory leak unless you
     * set "subimage" to false in the returned subimage
     */
    template<class E>
    Gray<E> create_subimage( const Gray<E>& input ) {
        Gray<E> *larger = new Gray<E>(input.width+4, input.height+6);
        Gray<E> sub = larger->makeSubimage(2,3,2+input.width,3+input.height);
        sub.copy(input);
//        sub.subimage = false;
        // hack it so it doesn't delete the array in the de-constructor
        larger->subimage = true;
        delete larger;
        return sub;
    }

    template<class E>
    void check_equals( const Gray<E>& imgA , const Gray<E>& imgB , E tol = test_tolerance<E>() ) {
        ASSERT_EQ(imgA.width , imgB.width);
        ASSERT_EQ(imgA.height , imgB.height);

        for( uint32_t y = 0; y < imgA.height; y++ ) {
            for( uint32_t x = 0; x < imgA.width; x++ ) {
                // TODO see if there's a built in function
                ASSERT_TRUE( std::abs(imgA.at(x,y)-imgB.at(x,y)) <= tol );
            }
        }
    }

    /// Floating point tolerance: 1e-5 for F32, 1e-12 for F64 and 0 for integers
    template<class E>
    double float_tolerance() {
        if (std::is_same<E, F32>::value)
            return 1e-5;
        if (std::is_same<E, F64>::value)
            return 1e-12;
        return 0.0;
    }

    /// Each pixel must satisfy |found - expected| <= tol*scale. A tolerance of 0 requires identical values. See
    /// CodeStandards.md, Testing Fast, before changing scale
    template<class E>
    void check_within( const Gray<E>& expected, const Gray<E>& found, double tol, double scale = 1.0 ) {
        ASSERT_EQ(expected.width, found.width);
        ASSERT_EQ(expected.height, found.height);
        for (uint32_t y = 0; y < expected.height; y++) {
            for (uint32_t x = 0; x < expected.width; x++) {
                double e = (double)expected.at(x, y);
                double f = (double)found.at(x, y);
                ASSERT_LE(std::abs(f - e), tol*scale) << "at (" << x << "," << y << ")";
            }
        }
    }

    template<class E>
    void check_equals_border( const Gray<E>& expected , const Gray<E>& found ,
                              uint32_t borderX0 , uint32_t borderX1 , uint32_t borderY0, uint32_t borderY1 ,
                              E tol = test_tolerance<E>() ) {
        ASSERT_EQ(expected.width , found.width);
        ASSERT_EQ(expected.height , found.height);

        for( int32_t y = 0; y < expected.height; y++ ) {
            for( int32_t x = 0; x < expected.width; x++ ) {
                if( x < borderX0 || y < borderY0 || x >= expected.width - borderX1 || y >= expected.height - borderY1 )
                {
                    bool matches = std::abs(expected.at(x,y)-found.at(x,y)) <= tol;
                    if( !matches ) {
                        printf("failed to match at %d %d\n",x,y);
                        std::cout << "  expected " << (int)expected.at(x,y) << " found " << (int)found.at(x,y)
                                  << std::endl << std::endl;
                    }

                    ASSERT_TRUE( matches );
                }
            }
        }
    }

    template<class E>
    void check_equals_inner( const Gray<E>& expected , const Gray<E>& found ,
                             uint32_t borderX0 , uint32_t borderX1 , uint32_t borderY0, uint32_t borderY1 ,
                             E tol = test_tolerance<E>() )
    {
        ASSERT_EQ(expected.width , found.width);
        ASSERT_EQ(expected.height , found.height);

        for( int32_t y = 0; y < expected.height; y++ ) {
            for( int32_t x = 0; x < expected.width; x++ ) {
                if( x >= borderX0 && y >= borderY0 && x < expected.width - borderX1 && y < expected.height - borderY1 )
                {
                    bool matches = std::abs(expected.at(x,y)-found.at(x,y)) <= tol;
                    if( !matches ) {
                        printf("failed to match at %d %d\n",x,y);
                        std::cout << "  expected " << (int)expected.at(x,y) << " found " << (int)found.at(x,y)
                                  << std::endl << std::endl;
                    }
                    ASSERT_TRUE( matches );
                }
            }
        }
    }

    /// Synthetic rectified stereo pair. The left image is random and the right image is the left shifted by 'shift'
    /// pixels plus uniform noise in [-10,10], so there are real matches at disparity 'shift'.
    inline void create_stereo_pair( Gray<U8>& left, Gray<U8>& right, uint32_t width, uint32_t height, int32_t shift,
                                    uint32_t seed = 0xBEEF ) {
        std::mt19937 gen(seed);
        left.reshape(width, height);
        right.reshape(width, height);
        ImageMiscOps::fill_uniform(left, (U8)0, (U8)255, gen);
        std::uniform_int_distribution<int32_t> noise(-10, 10);
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                int32_t src = std::min<int32_t>(width - 1, std::max<int32_t>(0, (int32_t)x + shift));
                int32_t v = (int32_t)left.at(src, y) + noise(gen);
                right.at(x, y) = (U8)std::max(0, std::min(255, v));
            }
        }
    }

}

#endif