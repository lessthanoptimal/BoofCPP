#ifndef BOOFCPP_THRESHOLD_BLOCK_REFERENCE_H
#define BOOFCPP_THRESHOLD_BLOCK_REFERENCE_H

#include "boofcv/base_types.h"
#include "boofcv/config_types.h"
#include "boofcv/image_types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

/// The block filters computed one block at a time, in the order of BoofCV's Java code: each block's statistics, then
/// each block's threshold from the statistics of its local region. Include a variant header, e.g.
/// variant_strict.h, first. Otsu uses tested::ComputeOtsu
namespace threshold_block_reference {
    using namespace boofcv;

    struct Blocks {
        uint32_t width, height;  // size of a block, except the last in each row and column
        uint32_t cols, rows;     // number of blocks
        bool local;

        Blocks( const ConfigLength& requested, bool local, uint32_t image_width, uint32_t image_height )
                : local(local) {
            auto r = (uint32_t)requested.computeI(std::min(image_width, image_height));
            width = image_width/(image_width/r);
            height = image_height/(image_height/r);
            cols = image_width/width;
            rows = image_height/height;
        }

        /// Calls stat(index, x0, y0, x1, y1) for each block, then threshold(index, x0, y0, x1, y1, region) where
        /// region lists the indexes of the blocks in its local region
        template<class Stat, class Threshold>
        void process( uint32_t image_width, uint32_t image_height, Stat stat, Threshold threshold ) const {
            auto x1 = [&]( uint32_t bx ) { return bx == cols - 1 ? image_width : (bx + 1)*width; };
            auto y1 = [&]( uint32_t by ) { return by == rows - 1 ? image_height : (by + 1)*height; };
            for (uint32_t by = 0; by < rows; by++)
                for (uint32_t bx = 0; bx < cols; bx++)
                    stat(by*cols + bx, bx*width, by*height, x1(bx), y1(by));
            for (uint32_t by = 0; by < rows; by++) {
                for (uint32_t bx = 0; bx < cols; bx++) {
                    std::vector<uint32_t> region;
                    const uint32_t r = local ? 1 : 0;
                    for (uint32_t y = by > r ? by - r : 0; y <= std::min(rows - 1, by + r); y++)
                        for (uint32_t x = bx > r ? bx - r : 0; x <= std::min(cols - 1, bx + r); x++)
                            region.push_back(y*cols + x);
                    threshold(bx*width, by*height, x1(bx), y1(by), region);
                }
            }
        }
    };

    /// Java's (int) cast
    int64_t java_int( double value ) {
        if (std::isnan(value))
            return 0;
        return (int64_t)std::clamp(std::trunc(value), (double)std::numeric_limits<int32_t>::min(),
                                   (double)std::numeric_limits<int32_t>::max());
    }

    /// BoofCV's U8 code computes with a double scale and F32's with a float scale
    template<class E>
    Gray<U8> mean( const Gray<E>& input, const ConfigLength& requested, bool local, double scale, bool down ) {
        typedef typename TypeInfo<E>::sum_type sum_type;
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<E> means(blocks.cols*blocks.rows);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            sum_type sum = 0;
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    sum += input.at(x, y);
            const uint32_t area = (x1 - x0)*(y1 - y0);
            if constexpr (std::is_integral_v<E>)
                means[index] = static_cast<E>(java_int(scale*sum/area + 0.5));  // like Java's (byte)
            else
                means[index] = (float)scale*sum/area;
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            sum_type sum = 0;
            for (uint32_t i : region)
                sum += means[i];
            const E threshold = sum/(uint32_t)region.size();
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    output.at(x, y) = (U8)(down == (input.at(x, y) <= threshold));
        });
        return output;
    }

    template<class E>
    Gray<U8> otsu( const Gray<E>& input, bool otsu2, const ConfigLength& requested, double tuning, double scale,
                   bool down, bool local ) {
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<uint32_t> histograms(blocks.cols*blocks.rows*256, 0);
        tested::ComputeOtsu otsu(otsu2, tuning, down, scale);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    histograms[index*256 + (U8)input.at(x, y)]++;
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            GrowArray<uint32_t> histogram(256);
            histogram.fill(0);
            uint32_t total = 0;
            for (uint32_t i : region) {
                for (uint32_t v = 0; v < 256; v++) {
                    histogram[v] += histograms[i*256 + v];
                    total += histograms[i*256 + v];
                }
            }
            otsu.compute(histogram, total);
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    output.at(x, y) = (U8)(down == ((U8)input.at(x, y) <= otsu.threshold));
        });
        return output;
    }

    /// The textureless spread as BoofCV computes it for integer images: -1 if negative, otherwise Math.round saturated
    /// to an int, with NaN as 0
    template<class E>
    auto spread( double minimumSpread ) {
        typedef typename TypeInfo<E>::sum_type sum_type;
        if constexpr (std::is_integral_v<E>) {
            if (minimumSpread < 0)
                return (int64_t)-1;
            if (std::isnan(minimumSpread))
                return (int64_t)0;
            // Math.round. x + 0.5 is exact in x86's long double for the doubles tested, so floor rounds ties up
            const long double v = std::floor((long double)minimumSpread + 0.5L);
            return (int64_t)std::min(v, (long double)std::numeric_limits<int32_t>::max());
        } else {
            return (sum_type)minimumSpread;
        }
    }

    template<class E>
    Gray<U8> min_max( const Gray<E>& input, double minimumSpread, const ConfigLength& requested, bool local,
                      double scale, bool down ) {
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<E> mins(blocks.cols*blocks.rows), maxs(blocks.cols*blocks.rows);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            E min = input.at(x0, y0), max = min;
            for (uint32_t y = y0; y < y1; y++) {
                for (uint32_t x = x0; x < x1; x++) {
                    const E value = input.at(x, y);
                    if (value < min)
                        min = value;
                    else if (value > max)
                        max = value;
                }
            }
            mins[index] = min;
            maxs[index] = max;
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            E min = std::numeric_limits<E>::max();
            E max = std::numeric_limits<E>::lowest();
            for (uint32_t i : region) {
                if (mins[i] < min)
                    min = mins[i];
                if (maxs[i] > max)
                    max = maxs[i];
            }
            const auto textureThreshold = spread<E>(minimumSpread);
            for (uint32_t y = y0; y < y1; y++) {
                for (uint32_t x = x0; x < x1; x++) {
                    if ((decltype(textureThreshold))(max - min) <= textureThreshold) {
                        output.at(x, y) = 1;
                    } else {
                        bool below;
                        if constexpr (std::is_integral_v<E>)
                            below = input.at(x, y) <= java_int(scale*((max + min)/2));
                        else
                            below = input.at(x, y) <= (float)scale*(max + min)/2.0f;
                        output.at(x, y) = (U8)(down == below);
                    }
                }
            }
        });
        return output;
    }
}

#endif
