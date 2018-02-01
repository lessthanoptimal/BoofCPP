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
            T* end = &ptr[image.total_pixels()];
            while( ptr != end ) {
                *ptr++ = value;
            }
        }

        template<class T>
        static void fill( Interleaved<T>& image , T value ) {
            T* ptr = image.data;
            T* end = &ptr[image.total_pixels()*image.num_bands];
            while( ptr != end ) {
                *ptr++ = value;
            }
        }

        /**
         * Fills an integer gray image using a uniform distribution.
         *
         * @param image Image that's to be filled
         * @param min_value minimum value (inclusive)
         * @param max_value  maximum value (exclusive)
         * @param rng The random number generator, e.g. std::mt19937
         */
        template<class T, class RNG>
        static void fill_uniform( Gray<T>& image , T min_value , T max_value , RNG& rng, typename std::enable_if<std::is_integral<T>::value >::type* = 0) {

            std::uniform_int_distribution<T> dis(min_value, max_value-1);

            T* ptr = image.data;
            T* end = &ptr[image.total_pixels()];
            while( ptr != end ) {
                *ptr++ = dis(rng);
            }
        }

        template<class T, class RNG>
        static void fill_uniform( Gray<T>& image , T min_value , T max_value , RNG& rng, typename std::enable_if<std::is_floating_point<T>::value >::type* = 0) {
            std::uniform_real_distribution<T> dis(min_value, max_value);

            T* ptr = image.data;
            T* end = &ptr[image.total_pixels()];
            while( ptr != end ) {
                *ptr++ = dis(rng);
            }
        }
    };
}

#endif