#ifndef BOOFCPP_FAST_CENSUS_TRANSFORM_H
#define BOOFCPP_FAST_CENSUS_TRANSFORM_H

#include "boofcv/base_types.h"
#include "boofcv/geometry_types.h"
#include "boofcv/image_border.h"
#include "boofcv/image_types.h"

#include <vector>

namespace boofcv::fast {
    /// Census transforms with the same results as strict's CensusTransform. Bit i is set if neighbor i is greater
    /// than the center pixel. The border pixels are strict's.
    ///
    /// @param border How pixels outside the image are read. If null the image border isn't modified.
    class CensusTransform {
    public:
        /// Census of the 3x3 region around each pixel. 8 bits, neighbors in row-major order. F32 uses strict's
        static void dense3x3( const Gray<U8>& input, Gray<U8>& output, ImageBorder<U8>* border );
        static void dense3x3( const Gray<U16>& input, Gray<U8>& output, ImageBorder<U16>* border );
        static void dense3x3( const Gray<F32>& input, Gray<U8>& output, ImageBorder<F32>* border );

        /// Census of the 5x5 region around each pixel. 24 bits, neighbors in row-major order
        static void dense5x5( const Gray<U8>& input, Gray<S32>& output, ImageBorder<U8>* border );
        static void dense5x5( const Gray<U16>& input, Gray<S32>& output, ImageBorder<U16>* border );
        static void dense5x5( const Gray<F32>& input, Gray<S32>& output, ImageBorder<F32>* border );

        /// Census of the given points around each pixel. Bit i is samples[i]. At most 64 samples. Without a border,
        /// pixels within the largest x or y of a sample from an edge aren't written
        static void sample_S64( const Gray<U8>& input, const std::vector<Point2D<int32_t>>& samples,
                                Gray<S64>& output, ImageBorder<U8>* border );
        static void sample_S64( const Gray<U16>& input, const std::vector<Point2D<int32_t>>& samples,
                                Gray<S64>& output, ImageBorder<U16>* border );
        static void sample_S64( const Gray<F32>& input, const std::vector<Point2D<int32_t>>& samples,
                                Gray<S64>& output, ImageBorder<F32>* border );
    };
}

#endif
