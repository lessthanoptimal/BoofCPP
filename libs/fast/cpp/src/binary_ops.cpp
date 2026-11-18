#include "boofcv/fast/binary_ops.h"
#include "boofcv/convolve.h"
#include "mean_window.h"

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

        /// Horizontal means of R rows, rounded as in strict. Prefix sums of each row are computed R rows at a time, so
        /// only their adds are serial and they overlap, then each window's sum is a difference of two prefix sums
        template<int R>
        void horizontal_means( const U8* const rows[], U8* const out[], uint32_t width, uint32_t radius,
                               std::vector<uint32_t>& prefix ) {
            const uint32_t stride = width + 1;
            prefix.resize(R*stride);
            uint32_t* p[R];
            uint32_t sum[R];
            for (int r = 0; r < R; r++) {
                p[r] = &prefix[r*stride];
                p[r][0] = 0;
                sum[r] = 0;
            }
            for (uint32_t x = 0; x < width; x++) {
                for (int r = 0; r < R; r++) {
                    sum[r] += rows[r][x];
                    p[r][x + 1] = sum[r];
                }
            }

            // Windows in [inner0, inner1) are inside the image
            const uint32_t inner0 = std::min(radius, width);
            const uint32_t inner1 = std::max(inner0, width > radius ? width - radius : 0);
            const MeanDivisor<U8> divisor(2*radius + 1);
            for (int r = 0; r < R; r++) {
                const uint32_t* prefix_r = p[r];
                U8* o = out[r];
                for (uint32_t x = 0; x < inner0; x++) {
                    const uint32_t upper = std::min(x + radius, width - 1) + 1;
                    o[x] = (U8)((prefix_r[upper] - prefix_r[0] + upper/2)/upper);
                }
                const uint32_t* upper = prefix_r + radius + 1;
                const uint32_t* lower = prefix_r - radius;
                if (divisor.uses_multiply32()) {
                    for (uint32_t x = inner0; x < inner1; x++)
                        o[x] = divisor.multiply32(upper[x] - lower[x]);
                } else {
                    for (uint32_t x = inner0; x < inner1; x++)
                        o[x] = divisor(upper[x] - lower[x]);
                }
                for (uint32_t x = inner1; x < width; x++) {
                    const uint32_t lower_x = x > radius ? x - radius : 0;
                    const uint32_t count = width - lower_x;
                    o[x] = (U8)((prefix_r[width] - prefix_r[lower_x] + count/2)/count);
                }
            }
        }

        /// Thresholds a row with its mean, as strict's ThresholdOps::localMean()
        template<class Mean>
        void threshold_row( const U8* in, U8* out, uint32_t width, float scale, bool down, Mean mean ) {
            if (down) {
                for (uint32_t x = 0; x < width; x++)
                    out[x] = static_cast<U8>(in[x] <= mean(x)*scale);
            } else {
                for (uint32_t x = 0; x < width; x++)
                    out[x] = static_cast<U8>(in[x]*scale > mean(x));
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

    void ThresholdOps::localMean( const Gray<U8>& input, Gray<U8>& output, const ConfigLength& width_config,
                                  float scale, bool down ) {
        output.reshape(input.width, input.height);
        const uint32_t radius = (uint32_t)width_config.computeI(std::min(input.width, input.height))/2;
        if (radius == 0)
            throw std::invalid_argument("Radius must be > 0");
        const uint32_t width = input.width, height = input.height;
        if (width == 0 || height == 0)
            return;
        // A window larger than the image covers the same pixels
        const uint32_t radius_x = std::min(radius, width);
        const uint32_t radius_y = std::min(radius, height);

        // Horizontal means of the rows in the vertical window, ROWS rows at a time. 'slots' rows are enough for the
        // window, the row leaving it and the rows computed ahead
        constexpr uint32_t ROWS = 4;
        const uint32_t slots = 2*radius_y + 1 + ROWS;
        std::vector<U8> means(slots*width);
        std::vector<uint32_t> prefix;
        uint32_t computed = 0;
        auto row_mean = [&]( uint32_t y ) { return &means[(y%slots)*width]; };
        auto compute_rows = [&]( uint32_t rows_needed ) {
            while (computed < rows_needed) {
                const uint32_t n = std::min(ROWS, height - computed);
                const U8* src[ROWS];
                U8* dst[ROWS];
                for (uint32_t r = 0; r < n; r++) {
                    src[r] = &input.data[input.offset + (computed + r)*input.stride];
                    dst[r] = row_mean(computed + r);
                }
                if (n == ROWS) {
                    horizontal_means<ROWS>(src, dst, width, radius_x, prefix);
                } else {
                    for (uint32_t r = 0; r < n; r++)
                        horizontal_means<1>(src + r, dst + r, width, radius_x, prefix);
                }
                computed += n;
            }
        };

        // Column sums of the horizontal means in the vertical window. Loops use their own copy of the width, which
        // isn't captured by the lambdas, so the compiler knows that stores to U8 pixels don't change it
        const uint32_t w = width;
        std::vector<uint32_t> totals(w, 0);
        uint32_t* sums = totals.data();
        compute_rows(radius_y);
        for (uint32_t y = 0; y < radius_y; y++) {
            const U8* m = row_mean(y);
            for (uint32_t x = 0; x < w; x++)
                sums[x] += m[x];
        }

        for (uint32_t y = 0; y < height; y++) {
            const U8* add = nullptr;
            const U8* remove = nullptr;
            if (y + radius_y < height) {
                compute_rows(y + radius_y + 1);
                add = row_mean(y + radius_y);
            }
            if (y > radius_y)
                remove = row_mean(y - radius_y - 1);

            const MeanDivisor<U8> divisor(window_count(y, radius_y, height));
            const U8* in = &input.data[input.offset + y*input.stride];
            U8* out = &output.data[output.offset + y*output.stride];

            // The lambdas below capture by value, so stores to U8 pixels can't change what they read
            if (add && remove && divisor.uses_multiply32()) {
                // Window inside the image: update the sums, divide and threshold in one loop
                threshold_row(in, out, w, scale, down, [=]( uint32_t x ) {
                    const uint32_t total = sums[x] + (add[x] - remove[x]);
                    sums[x] = total;
                    return divisor.multiply32(total);
                });
                continue;
            }
            if (add && remove) {
                for (uint32_t x = 0; x < w; x++)
                    sums[x] += add[x] - remove[x];
            } else if (add) {
                for (uint32_t x = 0; x < w; x++)
                    sums[x] += add[x];
            } else if (remove) {
                for (uint32_t x = 0; x < w; x++)
                    sums[x] -= remove[x];
            }
            threshold_row(in, out, w, scale, down, [=]( uint32_t x ) { return divisor(sums[x]); });
        }
    }
}
