#include "boofcv/fast/binary_ops.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace boofcv::fast {
    namespace {
        /// Same as strict's ImageStatistics::histogram() for U8. Pixels are counted in several histograms, so that
        /// repeated values don't wait on each other, which are then added up
        void histogram_U8( const Gray<U8>& input, int32_t minValue, GrowArray<uint32_t>& histogram ) {
            constexpr uint32_t COPIES = 4;
            std::vector<uint32_t> copies(256*COPIES, 0);
            uint32_t* h = copies.data();
            const uint32_t width = input.width;
            for (uint32_t y = 0; y < input.height; y++) {
                const U8* row = &input.data[input.offset + y*input.stride];
                uint32_t x = 0;
                for (; x + COPIES <= width; x += COPIES)
                    for (uint32_t k = 0; k < COPIES; k++)
                        h[256*k + row[x + k]]++;
                for (; x < width; x++)
                    h[row[x]]++;
            }

            histogram.fill(0);
            for (int32_t value = 0; value < 256; value++) {
                uint32_t count = 0;
                for (uint32_t k = 0; k < COPIES; k++)
                    count += h[256*k + value];
                if (count == 0)
                    continue;
                const int64_t bin = (int64_t)value - minValue;
                if (bin < 0 || bin >= histogram.size)
                    throw std::out_of_range("Pixel value outside of the histogram");
                histogram[(uint32_t)bin] = count;
            }
        }

    }

    uint32_t ThresholdOps::computeOtsu_U8( const Gray<U8>& input, double min_value, double max_value, bool otsu2 ) {
        // Same checks as strict's
        const double range = std::trunc(1 + max_value - min_value);
        if (!(range >= 1 && range <= std::numeric_limits<int32_t>::max()))
            throw std::invalid_argument("Bad histogram range");
        GrowArray<uint32_t> histogram((uint32_t)range);
        const double min_int = std::trunc(min_value);
        if (!(min_int >= std::numeric_limits<int32_t>::min() && min_int <= std::numeric_limits<int32_t>::max()))
            throw std::invalid_argument("min_value doesn't fit an int");
        histogram_U8(input, (int32_t)min_int, histogram);

        ComputeOtsu otsu(otsu2, 0, true, 1.0);
        otsu.compute(histogram, input.total_pixels());
        return (uint32_t)otsu.threshold;
    }
}
