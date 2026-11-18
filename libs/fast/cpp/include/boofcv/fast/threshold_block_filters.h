#ifndef BOOFCPP_FAST_THRESHOLD_BLOCK_FILTERS_H
#define BOOFCPP_FAST_THRESHOLD_BLOCK_FILTERS_H

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "boofcv/fast/binary_ops.h"
#include "boofcv/sanity_checks.h"
#include "boofcv/threshold_block_filters.h"

namespace boofcv::fast {
    using boofcv::ThresholdBlockOtsu;
    using boofcv::ThresholdBlockMinMax;

    /// Block threshold filters with the same results as strict's. Statistics are computed one row of blocks at a time,
    /// then that row of blocks is thresholded while its pixels are still in cache, one image row at a time with a
    /// threshold per column. With the local 3x3 region a row of blocks is thresholded once the next one has its
    /// statistics.
    ///
    /// The blocks are the same as strict's ThresholdBlockCommon: about requestedBlockWidth wide and tall, and the last
    /// block in each row and column also covers the pixels left over
    template<class E>
    class ThresholdBlockCommon : public InputToBinary<Gray<E>> {
    public:
        ConfigLength requestedBlockWidth;
        bool thresholdFromLocalBlocks;

        /// Size of a block and the number of blocks along x and y. Set by process()
        uint32_t blockWidth = 0, blockHeight = 0;
        uint32_t blocksX = 0, blocksY = 0;

        ThresholdBlockCommon( const ConfigLength& requestedBlockWidth, bool thresholdFromLocalBlocks )
                : requestedBlockWidth(requestedBlockWidth), thresholdFromLocalBlocks(thresholdFromLocalBlocks) {}

        void process( const Gray<E>& input, Gray<U8>& output ) override {
            checkSameShape(input, output);

            auto requested = (uint32_t)requestedBlockWidth.computeI(std::min(input.width, input.height));
            if (input.width < requested || input.height < requested)
                throw std::invalid_argument("Image is smaller than block size");
            if (requested == 0)
                throw std::invalid_argument("Block width must be at least 1");

            // Same as strict's selectBlockSize()
            blockHeight = input.height/(input.height/requested);
            blockWidth = input.width/(input.width/requested);
            blocksX = input.width/blockWidth;
            blocksY = input.height/blockHeight;

            begin(input.width);
            for (uint32_t by = 0; by < blocksY; by++) {
                statistics(input, by);
                if (!thresholdFromLocalBlocks)
                    threshold_rows(input, output, by);
                else if (by > 0)
                    threshold_rows(input, output, by - 1);
            }
            if (thresholdFromLocalBlocks)
                threshold_rows(input, output, blocksY - 1);
        }

    protected:
        uint32_t block_x0( uint32_t bx ) const { return bx*blockWidth; }
        uint32_t block_x1( uint32_t bx, uint32_t width ) const { return bx == blocksX - 1 ? width : (bx + 1)*blockWidth; }
        uint32_t block_y0( uint32_t by ) const { return by*blockHeight; }
        uint32_t block_y1( uint32_t by, uint32_t height ) const {
            return by == blocksY - 1 ? height : (by + 1)*blockHeight;
        }

        /// Blocks [bx0, bx1] x [by0, by1] whose statistics give block (bx, by)'s threshold
        void region( uint32_t bx, uint32_t by, uint32_t& bx0, uint32_t& by0, uint32_t& bx1, uint32_t& by1 ) const {
            if (thresholdFromLocalBlocks) {
                bx0 = bx > 0 ? bx - 1 : 0;
                by0 = by > 0 ? by - 1 : 0;
                bx1 = std::min(blocksX - 1, bx + 1);
                by1 = std::min(blocksY - 1, by + 1);
            } else {
                bx0 = bx1 = bx;
                by0 = by1 = by;
            }
        }

        /// Allocates storage for an image this wide, with the block sizes already selected
        virtual void begin( uint32_t width ) = 0;

        /// Statistics of every block in row 'by'
        virtual void statistics( const Gray<E>& input, uint32_t by ) = 0;

        /// Thresholds every pixel in the row of blocks 'by'
        virtual void threshold_rows( const Gray<E>& input, Gray<U8>& output, uint32_t by ) = 0;
    };

    /// Same results as strict's ThresholdBlockMean. Each block's mean is computed in strict's order. For floats,
    /// eight blocks are summed at once so their adds overlap
    template<class E>
    class ThresholdBlockMean : public ThresholdBlockCommon<E> {
    public:
        using sum_type = typename TypeInfo<E>::sum_type;

        // float like strict's
        float scale;
        bool down;

        ThresholdBlockMean( const ConfigLength& requestedBlockWidth, bool thresholdFromLocalBlocks, double scale,
                            bool down )
                : ThresholdBlockCommon<E>(requestedBlockWidth, thresholdFromLocalBlocks), scale((float)scale),
                  down(down) {}

    protected:
        std::vector<sum_type> sums;
        std::vector<E> means;
        std::vector<E> column;

        void begin( uint32_t width ) override {
            sums.resize(this->blocksX);
            means.resize(this->blocksX*this->blocksY);
            column.resize(width);
        }

        void statistics( const Gray<E>& input, uint32_t by ) override {
            const uint32_t blocks = this->blocksX, block_width = this->blockWidth, width = input.width;
            const uint32_t y0 = this->block_y0(by), y1 = this->block_y1(by, input.height);
            sum_type* s = sums.data();
            std::fill(s, s + blocks, (sum_type)0);
            for (uint32_t y = y0; y < y1; y++) {
                const E* row = &input.data[input.offset + y*input.stride];
                if constexpr (std::is_integral_v<E>) {
                    for (uint32_t bx = 0; bx < blocks; bx++) {
                        const uint32_t x1 = this->block_x1(bx, width);
                        sum_type total = 0;
                        for (uint32_t x = this->block_x0(bx); x < x1; x++)
                            total += row[x];
                        s[bx] += total;
                    }
                } else {
                    sum_float_row(row, s, blocks, block_width, width);
                }
            }
            for (uint32_t bx = 0; bx < blocks; bx++) {
                const uint32_t area = (this->block_x1(bx, width) - this->block_x0(bx))*(y1 - y0);
                sum_type sum = s[bx];
                if constexpr (std::is_integral_v<E>)
                    sum = static_cast<sum_type>(scale*sum/area + 0.5f);
                else
                    sum = static_cast<sum_type>(scale*sum/area);
                means[by*blocks + bx] = static_cast<E>(sum);
            }
        }

        /// Adds a row's pixels to each block's sum, one pixel after another as strict does. The last block also covers
        /// the pixels after the other blocks
        static void sum_float_row( const E* row, sum_type* s, uint32_t blocks, uint32_t block_width, uint32_t width ) {
            const uint32_t last = blocks - 1;
            uint32_t bx = 0;
            for (; bx + 8 <= last; bx += 8) {
                const E* r = row + bx*block_width;
                sum_type s0 = s[bx], s1 = s[bx + 1], s2 = s[bx + 2], s3 = s[bx + 3];
                sum_type s4 = s[bx + 4], s5 = s[bx + 5], s6 = s[bx + 6], s7 = s[bx + 7];
                for (uint32_t x = 0; x < block_width; x++) {
                    s0 += r[x];
                    s1 += r[block_width + x];
                    s2 += r[2*block_width + x];
                    s3 += r[3*block_width + x];
                    s4 += r[4*block_width + x];
                    s5 += r[5*block_width + x];
                    s6 += r[6*block_width + x];
                    s7 += r[7*block_width + x];
                }
                s[bx] = s0; s[bx + 1] = s1; s[bx + 2] = s2; s[bx + 3] = s3;
                s[bx + 4] = s4; s[bx + 5] = s5; s[bx + 6] = s6; s[bx + 7] = s7;
            }
            for (; bx < last; bx++) {
                const E* r = row + bx*block_width;
                for (uint32_t x = 0; x < block_width; x++)
                    s[bx] += r[x];
            }
            for (uint32_t x = last*block_width; x < width; x++)
                s[last] += row[x];
        }

        void threshold_rows( const Gray<E>& input, Gray<U8>& output, uint32_t by ) override {
            const uint32_t width = input.width;
            E* threshold = column.data();
            for (uint32_t bx = 0; bx < this->blocksX; bx++) {
                uint32_t bx0, by0, bx1, by1;
                this->region(bx, by, bx0, by0, bx1, by1);
                sum_type sum = 0;
                for (uint32_t y = by0; y <= by1; y++)
                    for (uint32_t x = bx0; x <= bx1; x++)
                        sum += means[y*this->blocksX + x];
                const E mean = sum/((by1 - by0 + 1)*(bx1 - bx0 + 1));
                std::fill(threshold + this->block_x0(bx), threshold + this->block_x1(bx, width), mean);
            }
            for (uint32_t y = this->block_y0(by); y < this->block_y1(by, input.height); y++) {
                const E* in = &input.data[input.offset + y*input.stride];
                U8* out = &output.data[output.offset + y*output.stride];
                if (down) {
                    for (uint32_t x = 0; x < width; x++)
                        out[x] = static_cast<U8>(in[x] <= threshold[x]);
                } else {
                    for (uint32_t x = 0; x < width; x++)
                        out[x] = static_cast<U8>(!(in[x] <= threshold[x]));
                }
            }
        }
    };

}

#endif
