#ifndef BOOFCPP_FAST_IMAGE_BLUR_H
#define BOOFCPP_FAST_IMAGE_BLUR_H

#include "boofcv/base_types.h"
#include "boofcv/image_border.h"
#include "boofcv/image_types.h"

#include <vector>

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

        /// Mean box filter where pixels outside the image are read from 'border', same as strict's
        /// BlurImageOps::mean() with a border. Radii larger than the image work.
        ///
        /// @param storage Work space, only used if output is the input
        /// @param totals Not used. Same signature as strict's
        static void mean( const Gray<F32>& input, Gray<F32>& output, uint32_t radius_x, uint32_t radius_y,
                          ImageBorder<F32>& border, Gray<F32>& storage, std::vector<F32>& totals );
        static void mean( const Gray<F64>& input, Gray<F64>& output, uint32_t radius_x, uint32_t radius_y,
                          ImageBorder<F64>& border, Gray<F64>& storage, std::vector<F64>& totals );
    };
}

#endif
