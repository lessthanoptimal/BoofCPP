#ifndef BOOFCPP_BLUR_IMAGE_H
#define BOOFCPP_BLUR_IMAGE_H

#include "images_types.h"

namespace boofcv {

    class BlurImage {
    public:
        template<class T>
        static void mean( const Gray<T>& input , Gray<T>& output, uint32_t radius , Gray<T>& storage ) {

        }
    };
}

#endif