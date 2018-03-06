#ifndef BOOFCPP_PRINT_STRUCTURES_H
#define BOOFCPP_PRINT_STRUCTURES_H

#include <stdio.h>
#include "convolve.h"
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
    void print( const Gray<E>& a , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {
        printf("image %d x %d\n",a.width,a.height);

        for( uint32_t y = 0; y < a.height; y++ ) {
            printf("[ ");
            for( uint32_t x = 0; x < a.width; x++ ) {
                printf("%02x ",a.at(x,y));
            }
            printf("]\n");
        }
        printf("\n");
    }

    template<typename E>
    void print( const Gray<E>& a , typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 ) {
        printf("image %d x %d\n",a.width,a.height);

        for( uint32_t y = 0; y < a.height; y++ ) {
            printf("[ ");
            for( uint32_t x = 0; x < a.width; x++ ) {
                printf("%.4e ",a.at(x,y));
            }
            printf("]\n");
        }
        printf("\n");
    }

    template<typename E>
    void print_kernel( const Kernel1D<E>& a , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {

        printf("Kernel1D[");
        for( uint32_t y = 0; y < a.width; y++ ) {
            printf(" %d",a[y]);
        }
        printf(" ]\n");
    }

    template<typename E>
    void print_kernel( const Kernel1D<E>& a , typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 ) {

        printf("Kernel1D[");
        for( uint32_t y = 0; y < a.width; y++ ) {
            printf(" %.4e",a[y]);
        }
        printf(" ]\n");
    }

    template<typename E>
    void print_kernel( const Kernel2D<E>& a , typename std::enable_if<std::is_integral<E>::value>::type* = 0 ) {

        printf("Kernel2D{ offset=%d, width=%d }\n",a.offset,a.width);
        for( uint32_t y = 0; y < a.width; y++ ) {
            printf(" ");
            for( uint32_t x = 0; x < a.width; x++ ) {
                printf(" %d", a.data[y*a.width+x]);
            }
            printf("\n");
        }
    }

    template<typename E>
    void print_kernel( const Kernel2D<E>& a , typename std::enable_if<std::is_floating_point<E>::value>::type* = 0 ) {

        printf("Kernel2D{ offset=%d, width=%d }\n",a.offset,a.width);
        for( uint32_t y = 0; y < a.width; y++ ) {
            printf(" ");
            for( uint32_t x = 0; x < a.width; x++ ) {
                printf(" %.4e", a.data[y*a.width+x]);
            }
            printf("\n");
        }
    }
}

#endif