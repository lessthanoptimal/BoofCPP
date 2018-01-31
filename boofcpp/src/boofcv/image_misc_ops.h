#ifndef BOOFCPP_IMAGE_MISC_OPS_H
#define BOOFCPP_IMAGE_MISC_OPS_H

#include <random>
#include "images_types.h"

namespace boofcv {
    class ImageMiscOps {
    public:
        template<class T>
        static void fill( Gray<T>& image , T value ) {
            T* ptr = image.data;
            T* end = &ptr[image.numberOfPixels()];
            while( ptr != end ) {
                *ptr++ = value;
            }
        }

//        template<class T, class RNG>
//        static void fill_uniform( Gray<T>& image , T min_value , T max_value , RNG& rng typename std::enable_if<std::is_integral<T>::value >::type* = 0) {
//
//            std::uniform_int_distribution<T> dis(min_value, max_value);
//
//            const T* ptr = input.data;
//            const T* end = &ptr[input.numberOfPixels()];
//            while( ptr != end ) {
//                *ptr++ = dis(gen);
//            }
//        }
//
//        template<class T, class RNG>
//        static void fill_uniform( Gray<T>& image , T min_value , T max_value , RNG& rng, typename std::enable_if<std::is_floating_point<T>::value >::type* = 0) {
//            std::uniform_real_distribution<T> dis(min_value, max_value);
//
//            const T* ptr = input.data;
//            const T* end = &ptr[input.numberOfPixels()];
//            while( ptr != end ) {
//                *ptr++ = dis(gen);
//            }
//        }
    };
}

#endif