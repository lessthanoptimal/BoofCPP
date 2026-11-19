#include "boofcv/fast/binary_ops.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace boofcv::fast {
    void ComputeOtsuNonEmpty::compute( const uint32_t* bins, const uint32_t* counts, uint32_t n, uint32_t length,
                                       uint32_t totalPixels ) {
        // Same arithmetic as strict's ComputeOtsu::computeOtsu() and computeOtsu2()
        const double dlength = length;
        double sum = 0;
        for (uint32_t j = 0; j < n; j++)
            sum += (bins[j]/dlength)*counts[j];

        double sumB = 0;
        uint32_t wB = 0;
        variance = 0;
        threshold = 0;
        double selectedMB = 0;
        double selectedMF = 0;

        for (uint32_t j = 0; j < n; j++) {
            wB += counts[j];                 // Weight Background
            uint32_t wF = totalPixels - wB;  // Weight Foreground
            if (wF == 0)
                break;

            double f = bins[j]/dlength;
            sumB += f*counts[j];

            double mB = sumB/wB;             // Mean Background
            double mF = (sum - sumB)/wF;     // Mean Foreground

            double varBetween = (double)wB*(double)wF*(mB - mF)*(mB - mF);
            if (varBetween > variance) {
                variance = varBetween;
                if (useOtsu2) {
                    selectedMB = mB;
                    selectedMF = mF;
                } else {
                    threshold = bins[j];
                }
            }
        }
        if (useOtsu2)
            threshold = length*(selectedMB + selectedMF)/2.0;

        // Same as strict's ComputeOtsu::compute()
        variance += 0.001;
        int adjustment = (int)(tuning*threshold*tuning*threshold/variance + 0.5);
        threshold += down ? -adjustment : adjustment;
        threshold = (int)(scale*std::max(threshold, 0.0) + 0.5);
    }
}
