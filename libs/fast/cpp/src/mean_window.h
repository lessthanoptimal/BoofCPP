#ifndef BOOFCPP_FAST_MEAN_WINDOW_H
#define BOOFCPP_FAST_MEAN_WINDOW_H

#include <algorithm>
#include <cstdint>

namespace boofcv::fast {
    /// Number of pixels inside the image in the window [i - radius, i + radius]
    inline uint32_t window_count( uint32_t i, uint32_t radius, uint32_t length ) {
        uint32_t lower = i > radius ? i - radius : 0;
        uint32_t upper = std::min(i + radius, length - 1);
        return upper - lower + 1;
    }
}

#endif
