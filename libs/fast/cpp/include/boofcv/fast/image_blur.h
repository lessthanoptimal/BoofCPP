#ifndef BOOFCPP_FAST_IMAGE_BLUR_H
#define BOOFCPP_FAST_IMAGE_BLUR_H

#include "boofcv/base_types.h"
#include "boofcv/image_types.h"

namespace boofcv::fast {
    /// Blur filters with results within floating point tolerance of BoofCV's BlurImageOps
    class BlurImageOps {
    public:
        /// Mean box filter, same as strict's BlurImageOps::mean(). Near the image border the box only covers pixels
        /// inside the image.
        ///
        /// @param storage Work space, only used if output is the input
        static void mean( const Gray<F32>& input, Gray<F32>& output, uint32_t radius, Gray<F32>& storage );
        static void mean( const Gray<F64>& input, Gray<F64>& output, uint32_t radius, Gray<F64>& storage );

        /// Mean box filter with a different radius along x and y. See mean() above
        static void mean( const Gray<F32>& input, Gray<F32>& output, uint32_t radius_x, uint32_t radius_y,
                          Gray<F32>& storage );
        static void mean( const Gray<F64>& input, Gray<F64>& output, uint32_t radius_x, uint32_t radius_y,
                          Gray<F64>& storage );
    };
}

#endif
