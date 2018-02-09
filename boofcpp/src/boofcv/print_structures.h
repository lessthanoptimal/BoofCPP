#ifndef BOOFCPP_PRINT_STRUCTURES_H
#define BOOFCPP_PRINT_STRUCTURES_H

#include <stdio.h>
#include "sanity_checks.h"
#include "testing_utils.h"

namespace boofcv {
    template<typename E>
    void print_difference( const Gray<E>& a , const Gray<E>& b , E tol = test_tolerance<E>() ) {
        checkSameShape(a,b);

        for( uint32_t y = 0; y < a.height; y++ ) {
            for( uint32_t x = 0; x < a.width; x++ ) {
                if( std::abs(a.at(x,y) - b.at(x,y)) > tol ) {
                    printf("X");
                } else {
                    printf("-");
                }
            }
            printf("\n");
        }
    }

    template<typename E>
    void print_kernel( const Kernel1D<E>& a , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {

        printf("Kernel1D[");
        for( uint32_t y = 0; y < a.width; y++ ) {
            printf(" %d",a[y]);
        }
        printf(" ]\n");
    }
}

#endif