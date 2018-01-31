#ifndef BOOFCPP_TESTING_UTILS_H
#define BOOFCPP_TESTING_UTILS_H

#include "gtest/gtest.h"
#include "base_types.h"
#include "images_types.h"

namespace boofcv {

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
    void check_equals( const Gray<E>& imgA , const Gray<E>& imgB , E tol = 0 ) {
        ASSERT_EQ(imgA.width , imgB.width);
        ASSERT_EQ(imgA.height , imgB.height);

        for( uint32_t y = 0; y < imgA.height; y++ ) {
            for( uint32_t x = 0; x < imgA.width; x++ ) {
                // TODO see if there's a built in function
                ASSERT_TRUE( std::abs(imgA.at(x,y)-imgB.at(x,y)) <= tol );
            }
        }
    }
}

#endif