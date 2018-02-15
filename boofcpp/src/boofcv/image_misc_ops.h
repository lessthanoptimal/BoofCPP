#ifndef BOOFCPP_IMAGE_MISC_OPS_H
#define BOOFCPP_IMAGE_MISC_OPS_H

#include <random>
#include "image_types.h"

namespace boofcv {
    class ImageMiscOps {
    public:
        template<class E>
        static void fill( Gray<E>& image , E value ) {
            if( image.subimage ) {
                for( uint32_t y = 0; y < image.height; y++ ) {
                    E *ptr = &image.data[image.offset + y*image.stride];
                    E *end = &ptr[image.width];
                    while (ptr != end) {
                        *ptr++ = value;
                    }
                }
            } else {
                E *ptr = image.data;
                E *end = &ptr[image.total_pixels()];
                while (ptr != end) {
                    *ptr++ = value;
                }
            }
        }

        template<class E>
        static void fill_border( Gray<E>& input , E value , uint32_t radius  ) {
            if( radius >= input.width || radius >= input.height ) {
                fill(input,value);
                return;
            }
            // top and bottom
            for (uint32_t y = 0; y < radius; y++) {
                uint32_t indexTop = input.offset + y * input.stride;
                uint32_t indexBottom = input.offset + (input.height-y-1) * input.stride;
                for (uint32_t x = 0; x < input.width; x++) {
                    input.data[indexTop++] = value;
                    input.data[indexBottom++] = value;
                }
            }

            // left and right
            uint32_t h = input.height-radius;
            uint32_t indexStart = input.offset + radius*input.stride;
            for (uint32_t x = 0; x < radius; x++) {
                uint32_t indexLeft = indexStart + x;
                uint32_t indexRight = indexStart + input.width-1-x;
                for (uint32_t y = radius; y < h; y++) {
                    input.data[indexLeft] = value;
                    input.data[indexRight] = value;

                    indexLeft += input.stride;
                    indexRight += input.stride;
                }
            }
        }

        template<class E>
        static void fill( Interleaved<E>& image , E value ) {
            E* ptr = image.data;
            E* end = &ptr[image.total_pixels()*image.num_bands];
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
        template<class E, class RNG>
        static void fill_uniform( Gray<E>& image , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_integral<E>::value >::type* = 0) {

            std::uniform_int_distribution<E> dis(min_value, max_value-1);

            E* ptr = image.data;
            E* end = &ptr[image.total_pixels()];
            while( ptr != end ) {
                *ptr++ = dis(rng);
            }
        }

        template<class E, class RNG>
        static void fill_uniform( Gray<E>& image , E min_value , E max_value , RNG& rng, typename std::enable_if<std::is_floating_point<E>::value >::type* = 0) {
            std::uniform_real_distribution<E> dis(min_value, max_value);

            E* ptr = image.data;
            E* end = &ptr[image.total_pixels()];
            while( ptr != end ) {
                *ptr++ = dis(rng);
            }
        }
    };
}

#endif