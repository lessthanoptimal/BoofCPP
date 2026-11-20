#include <algorithm>
#include <stdexcept>
#include <vector>

#include "boofcv/binary_ops.h"
#include "boofcv/sanity_checks.h"

using namespace boofcv;

void boofcv::BinaryImageOps::logicAnd(const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output )
{
    checkSameShape(inputA, inputB);
    output.reshape(inputA.width,inputA.height);

    for( uint32_t y = 0; y < inputA.height; y++ ) {
        uint32_t indexA = inputA.offset + y*inputA.stride;
        uint32_t indexB = inputB.offset + y*inputB.stride;
        uint32_t indexOut = output.offset + y*output.stride;

        int end = indexA + inputA.width;
        for( ; indexA < end; indexA++,indexB++,indexOut++) {
            output.data[indexOut] = static_cast<U8>(inputA.data[indexA] && inputB.data[indexB]);
        }
    }
}

void boofcv::BinaryImageOps::logicOr(const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output )
{
    checkSameShape(inputA, inputB);
    output.reshape(inputA.width,inputA.height);

    for( uint32_t y = 0; y < inputA.height; y++ ) {
        uint32_t indexA = inputA.offset + y*inputA.stride;
        uint32_t indexB = inputB.offset + y*inputB.stride;
        uint32_t indexOut = output.offset + y*output.stride;

        uint32_t end = indexA + inputA.width;
        for( ; indexA < end; indexA++,indexB++,indexOut++) {
            output.data[indexOut] = static_cast<U8>(inputA.data[indexA] || inputB.data[indexB]);
        }
    }
}

void boofcv::BinaryImageOps::erode4(const Gray<U8>& input, Gray<U8>& output) {
    output.reshape(input.width,input.height);

    const int h = input.height - 1;
    const int w = input.width - 2;

    for (uint32_t y = 1; y < h; y++) {
        uint32_t indexIn = input.offset + y * input.stride + 1;
        uint32_t indexOut = output.offset + y * output.stride + 1;

        const uint32_t end = indexIn + w;

        for (; indexIn < end; indexIn++, indexOut++) {
            if ((input.data[indexIn] + input.data[indexIn - 1] + input.data[indexIn + 1] +
                 input.data[indexIn - input.stride] + input.data[indexIn + input.stride]) == 5)
                output.data[indexOut] = 1;
            else
                output.data[indexOut] = 0;
        }
    }
}

void boofcv::BinaryImageOps::dilate4(const Gray<U8>& input, Gray<U8>& output) {
    output.reshape(input.width,input.height);

    const int h = input.height - 1;
    const int w = input.width - 2;

    for (uint32_t y = 1; y < h; y++) {
        uint32_t indexIn = input.offset + y * input.stride + 1;
        uint32_t indexOut = output.offset + y * output.stride + 1;

        const uint32_t end = indexIn + w;

        for (; indexIn < end; indexIn++, indexOut++) {
            if ((input.data[indexIn] +
                 input.data[indexIn - 1] + input.data[indexIn + 1] +
                 input.data[indexIn - input.stride] + input.data[indexIn + input.stride]) > 0)
                output.data[indexOut] = 1;
            else
                output.data[indexOut] = 0;
        }
    }
}

namespace {
    /// Otsu's last step in BoofCV: penalizes regions with little texture by 'tuning', then scales the threshold.
    /// The int casts are Java's, which saturate where a C++ cast is undefined. That only happens for an absurd tuning
    /// or scale, whose binary image is useless either way
    void otsu_tune_and_scale( double& threshold, double& variance, double tuning, bool down, double scale ) {
        variance += 0.001; // avoid divide by zero
        // multiply by threshold twice in an effort to have the image's scaling not effect the tuning parameter
        const int32_t adjustment = boofcv::java_int(tuning*threshold*tuning*threshold/variance + 0.5);
        threshold += down ? -(double)adjustment : (double)adjustment;
        threshold = boofcv::java_int(scale*std::max(threshold, 0.0) + 0.5);
    }
}

boofcv::ComputeOtsu::ComputeOtsu(bool useOtsu2, double tuning, bool down, double scale) {
    this->useOtsu2 = useOtsu2;
    this->tuning = tuning;
    this->down = down;
    this->scale = scale;
}

boofcv::ComputeOtsu::ComputeOtsu( bool useOtsu2 , bool down ) : ComputeOtsu(useOtsu2,0,down,1.0) {
}

void boofcv::ComputeOtsu::compute(const GrowArray<uint32_t>&histogram , uint32_t totalPixels) {
    if( useOtsu2 ) {
        computeOtsu2(histogram,totalPixels);
    } else {
        computeOtsu(histogram,totalPixels);
    }

    otsu_tune_and_scale(threshold, variance, tuning, down, scale);
}

void boofcv::ComputeOtsu::computeOtsu(const GrowArray<uint32_t>& histogram , uint32_t totalPixels ) {
    double dlength = histogram.size;
    double sum = 0;
    for (uint32_t i = 0; i < histogram.size; i++)
        sum += (i / dlength) * histogram[i];

    double sumB = 0;
    uint32_t wB = 0;

    variance = 0;
    threshold = 0;

    uint32_t i;
    for (i = 0; i < histogram.size; i++) {
        wB += histogram[i];               // Weight Background
        if (wB == 0) continue;

        uint32_t wF = totalPixels - wB;        // Weight Foreground
        if (wF == 0) break;

        double f = i / dlength;
        sumB += f * histogram[i];

        double mB = sumB / wB;            // Mean Background
        double mF = (sum - sumB) / wF;    // Mean Foreground

        // Calculate Between Class Variance
        double varBetween = (double) wB * (double) wF * (mB - mF) * (mB - mF);

        // Check if new maximum found
        if (varBetween > variance) {
            variance = varBetween;
            threshold = i;
        }
    }
}

void boofcv::ComputeOtsu::computeOtsu2(const GrowArray<uint32_t>&histogram , uint32_t totalPixels ) {
    double dlength = histogram.size;
    double sum = 0;
    for (int i = 0; i < histogram.size; i++)
        sum += (i / dlength) * histogram[i];

    double sumB = 0;
    uint32_t wB = 0;

    variance = 0;
    threshold = 0;

    double selectedMB=0;
    double selectedMF=0;

    uint32_t i;
    for (i = 0; i < histogram.size; i++) {
        wB += histogram[i];               // Weight Background
        if (wB == 0) continue;

        uint32_t wF = totalPixels - wB;        // Weight Foreground
        if (wF == 0) break;

        double f = i / dlength;
        sumB += f * histogram[i];

        double mB = sumB / wB;            // Mean Background
        double mF = (sum - sumB) / wF;    // Mean Foreground

        // Calculate Between Class Variance
        double varBetween = (double) wB * (double) wF * (mB - mF) * (mB - mF);

        // Check if new maximum found
        if (varBetween > variance) {
            variance = varBetween;
            selectedMB = mB;
            selectedMF = mF;
        }
    }

    // select a threshold which maximizes the distance between the two distributions. In pathological
    // cases there's a dead zone where all the values are equally good and it would select a value with a low index
    // arbitrarily. Then if you scaled the threshold it would reject everything
    threshold = histogram.size*(selectedMB+selectedMF)/2.0;
}

namespace boofcv {
    void ComputeOtsuNonEmpty::compute( const uint32_t* bins, const uint32_t* counts, uint32_t n, uint32_t length,
                                       uint32_t totalPixels ) {
        // Same arithmetic as ComputeOtsu::computeOtsu() and computeOtsu2()
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

        otsu_tune_and_scale(threshold, variance, tuning, down, scale);
    }

    namespace {
        /// Horizontal means of R rows, rounded as in BlurImageOps::mean(). Prefix sums of each row are computed R rows
        /// at a time, so only their adds are serial and they overlap, then each window's sum is a difference of two
        /// prefix sums
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

        /// Thresholds a row with its mean, as ThresholdOps::localMean()
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

    void ThresholdOps::localMean_U8( const Gray<U8>& input, Gray<U8>& output, const ConfigLength& width_config,
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
