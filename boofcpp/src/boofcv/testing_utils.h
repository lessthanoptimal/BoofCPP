#ifndef BOOFCPP_TESTING_UTILS_H
#define BOOFCPP_TESTING_UTILS_H

#include "gtest/gtest.h"
#include "base_types.h"
#include "image_types.h"

namespace boofcv {

    /**
     * Selects a reasonable tolerance for equality
     * @tparam E
     * @return tolerance for equality
     */
    template<typename E>
    E test_tolerance() {
        if( std::numeric_limits<E>::is_exact ) {
            return (E)0;
        } else {
            return std::sqrt(std::numeric_limits<E>::epsilon());
        }
    }

    /**
     * Returns a subimage which is equivalent to the input image. There will be a memory leak unless you
     * set "subimage" to false in the returned subimage
     */
    template<class E>
    Gray<E> create_subimage( const Gray<E>& input ) {
        Gray<E> *larger = new Gray<E>(input.width+4, input.height+6);
        Gray<E> sub = larger->makeSubimage(2,3,2+input.width,3+input.height);
        sub.setTo(input);
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

    template<class E>
    void check_equals_border( const Gray<E>& expected , const Gray<E>& found ,
                              uint32_t borderX0 , uint32_t borderX1 , uint32_t borderY0, uint32_t borderY1 ,
                              E tol = test_tolerance<E>() ) {
        ASSERT_EQ(expected.width , found.width);
        ASSERT_EQ(expected.height , found.height);

        for( int y = 0; y < expected.height; y++ ) {
            for( int x = 0; x < expected.width; x++ ) {
                if( x < borderX0 || y < borderY0 || x >= expected.width - borderX1 || y >= expected.height - borderY1 )
                {
                    ASSERT_TRUE( std::abs(expected.at(x,y)-found.at(x,y)) <= tol );
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

        for( int y = 0; y < expected.height; y++ ) {
            for( int x = 0; x < expected.width; x++ ) {
                if( x >= borderX0 && y >= borderY0 && x < expected.width - borderX1 && y < expected.height - borderY1 )
                {
                    ASSERT_TRUE( std::abs(expected.at(x,y)-found.at(x,y)) <= tol );
                }
            }
        }
    }

}

#endif