#ifndef BOOFCPP_THRESHOLD_BLOCK_FILTERS_H
#define BOOFCPP_THRESHOLD_BLOCK_FILTERS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "boofcv/image_types.h"
#include "boofcv/base_types.h"
#include "boofcv/config_types.h"
#include "boofcv/sanity_checks.h"
#include "boofcv/binary_ops.h"

namespace boofcv
{
    /// Computes image statistics in regularly spaced blocks across the image. Then computes an average
    /// of the statistics in a block within a local 3x3 grid region. The average statistics in a local 3x3 grid
    /// region is used to reduce the adverse affects of using a grid.
    /// Ideally a local region around each pixel would be used, but this is expensive to compute.  Since a grid is
    /// used instead of a pixel local region boundary conditions can be an issue.  For example, consider a black square
    /// in the image, if the grid just happens to lie on this black square perfectly then if you look at only a single
    /// grid element it will be considered textureless and the edge lost.  This problem isn't an issue if you consder
    /// a local 3x3 region of blocks.
    ///
    /// The size each block in the grid in pixels is adjusted depending on image size.  This is done to minimize
    /// "squares" in the upper image boundaries from having many more pixels than other blocks.
    ///
    /// The block based approach used here was inspired by a high level description found in AprilTags.
    ///
    /// A row of blocks is thresholded while its pixels are still in cache:
    ///
    ///     for each row of blocks r:
    ///         statistics(r): the statistics of every block in r
    ///         threshold_rows(r), or with local blocks threshold_rows(r - 1) once r > 0
    ///     with local blocks: threshold_rows(last row of blocks)
    ///
    ///     threshold_rows(r):
    ///         for each block b in r:  column[x] = threshold from the statistics of region(b), for b's columns x
    ///         for each image row in r:  out[x] = down ? in[x] <= column[x] : in[x] > column[x]
    ///
    /// region(b) is b and its neighbors in a 3x3 grid of blocks with local blocks, otherwise only b. The last block of
    /// each row and column extends to the image's edge.
    ///
    /// @tparam E Type of the input image's pixels.
    ///
    /// @author Peter Abeles
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

            selectBlockSize(input.width, input.height, requested);
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

        /// Selects a block size which is close to the requested block size, so that the last block in each row and
        /// column doesn't have many more pixels than the others
        void selectBlockSize( uint32_t width, uint32_t height, uint32_t requestedBlockWidth ) {
            blockHeight = height/(height/requestedBlockWidth);
            blockWidth = width/(width/requestedBlockWidth);
        }

        bool isThresholdFromLocalBlocks() const { return thresholdFromLocalBlocks; }

        void setThresholdFromLocalBlocks( bool thresholdFromLocalBlocks ) {
            this->thresholdFromLocalBlocks = thresholdFromLocalBlocks;
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

    /// Applies a threshold to an image by computing the mean values in a regular grid across
    /// the image.  When thresholding all the pixels inside a box (grid element) the mean values is found
    /// in the surrounding 3x3 grid region.
    ///
    /// See ThresholdBlockMinMax for a more detailed discussion of elements of this strategy
    ///
    ///     statistics(r):
    ///         for each image row in r:  sum[b] += b's pixels in the row, left to right  (floats: 8 blocks at once)
    ///         mean[b] = scale*sum[b]/area
    ///     threshold of b:  the average of mean[] over region(b)
    ///
    /// @author Peter Abeles
    template<class E>
    class ThresholdBlockMean : public ThresholdBlockCommon<E> {
    public:
        using sum_type = typename TypeInfo<E>::sum_type;

        // Integer images compute with a double scale and others with a float scale
        double scale;
        bool down;

        ThresholdBlockMean( const ConfigLength& requestedBlockWidth, bool thresholdFromLocalBlocks, double scale,
                            bool down )
                : ThresholdBlockCommon<E>(requestedBlockWidth, thresholdFromLocalBlocks), scale(scale),
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
                if constexpr (std::is_integral_v<E>) {
                    // the int keeps only the bits that fit the pixel type
                    means[by*blocks + bx] = static_cast<E>(java_int(scale*s[bx]/area + 0.5));
                } else {
                    means[by*blocks + bx] = static_cast<float>(scale)*s[bx]/area;
                }
            }
        }

        /// Adds a row's pixels to each block's sum, one pixel after another. The last block also covers
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

    /// Block Otsu threshold implementation based on ThresholdBlockCommon. Computes a histogram in non-overlapping
    /// square regions. Then thresholds a single region by combining histograms from its neighbors to make it less blocky.
    ///
    /// This implementation includes a modification from the traditional Otsu algorithm. The threshold can optionally
    /// be adjusted in low variance regions. See code for details.
    ///
    /// NOTE: This produces visually different results from ThresholdBlockOtsu because the block algorithm
    /// combines histograms from its neighboring blocks. That's why it appears to have a wider effective block.
    ///
    ///     statistics(r):
    ///         for each image row in r:  add the row's pixels, as U8, to b's COPIES histograms, cycling through them
    ///         histogram[b] = the sum of b's copies
    ///     threshold of b:  ComputeOtsuNonEmpty over the non-empty bins of the sum of histogram[] over region(b)
    ///     pixels are compared as U8
    ///
    /// The copies keep repeated values from waiting on each other. Only the last ROWS rows of blocks are kept.
    ///
    /// @see GThresholdImageOps#computeOtsu(ImageGray, double, double)
    ///
    /// @author Peter Abeles
    template<class E>
    class ThresholdBlockOtsu : public ThresholdBlockCommon<E> {
    public:
        ComputeOtsuNonEmpty otsu;

        /// @param requestedBlockWidth About how wide and tall you wish a block to be in pixels.
        /// @param tuning Tuning parameter. 0 = standard Otsu. Greater than 0 will penalize zero texture.
        ThresholdBlockOtsu( bool otsu2, ConfigLength requestedBlockWidth, double tuning, double scale, bool down,
                            bool thresholdFromLocalBlocks )
                : ThresholdBlockCommon<E>(requestedBlockWidth, thresholdFromLocalBlocks),
                  otsu(otsu2, tuning, down, scale) {}

    protected:
        /// Copies of each histogram while it's being filled
        static constexpr uint32_t COPIES = std::is_same_v<E, U8> ? 2 : 4;
        /// Rows of blocks kept: the one being filled and the two before it
        static constexpr uint32_t ROWS = 3;

        std::vector<uint32_t> histograms;  // 256 bins per block for ROWS rows of blocks
        std::vector<uint32_t> copies;      // 256*COPIES bins per block for one row of blocks
        std::vector<U8> bytes;             // a row of pixels converted to U8
        std::vector<int16_t> column;       // threshold per column
        uint32_t histogram[256], bins[256], counts[256];

        void begin( uint32_t width ) override {
            histograms.resize(ROWS*this->blocksX*256);
            copies.assign(this->blocksX*256*COPIES, 0);
            if constexpr (!std::is_same_v<E, U8>)
                bytes.resize(width);
            column.resize(width);
        }

        uint32_t* block_histogram( uint32_t bx, uint32_t by ) {
            return &histograms[((by%ROWS)*this->blocksX + bx)*256];
        }

        void statistics( const Gray<E>& input, uint32_t by ) override {
            const uint32_t blocks = this->blocksX, width = input.width;
            for (uint32_t y = this->block_y0(by); y < this->block_y1(by, input.height); y++) {
                const U8* row;
                if constexpr (std::is_same_v<E, U8>) {
                    row = &input.data[input.offset + y*input.stride];
                } else {
                    const E* in = &input.data[input.offset + y*input.stride];
                    U8* b = bytes.data();
                    for (uint32_t x = 0; x < width; x++)
                        b[x] = U8(in[x]);
                    row = b;
                }
                for (uint32_t bx = 0; bx < blocks; bx++) {
                    uint32_t* h = &copies[bx*256*COPIES];
                    const uint32_t x1 = this->block_x1(bx, width);
                    uint32_t x = this->block_x0(bx);
                    for (; x + COPIES <= x1; x += COPIES)
                        for (uint32_t k = 0; k < COPIES; k++)
                            h[256*k + row[x + k]]++;
                    for (; x < x1; x++)
                        h[row[x]]++;
                }
            }
            for (uint32_t bx = 0; bx < blocks; bx++) {
                uint32_t* h = &copies[bx*256*COPIES];
                uint32_t* merged = block_histogram(bx, by);
                for (uint32_t i = 0; i < 256; i++) {
                    uint32_t total = 0;
                    for (uint32_t k = 0; k < COPIES; k++) {
                        total += h[256*k + i];
                        h[256*k + i] = 0;
                    }
                    merged[i] = total;
                }
            }
        }

        void threshold_rows( const Gray<E>& input, Gray<U8>& output, uint32_t by ) override {
            const uint32_t width = input.width;
            int16_t* threshold = column.data();
            for (uint32_t bx = 0; bx < this->blocksX; bx++) {
                uint32_t bx0, by0, bx1, by1;
                this->region(bx, by, bx0, by0, bx1, by1);
                std::fill(histogram, histogram + 256, 0);
                for (uint32_t y = by0; y <= by1; y++) {
                    for (uint32_t x = bx0; x <= bx1; x++) {
                        const uint32_t* h = block_histogram(x, y);
                        for (uint32_t i = 0; i < 256; i++)
                            histogram[i] += h[i];
                    }
                }
                uint32_t n = 0, total = 0;
                for (uint32_t i = 0; i < 256; i++) {
                    bins[n] = i;
                    counts[n] = histogram[i];
                    total += histogram[i];
                    n += histogram[i] != 0;
                }
                otsu.compute(bins, counts, n, 256, total);
                // The threshold is a whole number, so comparing a pixel with it as an integer is the same
                const auto t = (int16_t)std::min(256.0, std::max(-1.0, otsu.threshold));
                std::fill(threshold + this->block_x0(bx), threshold + this->block_x1(bx, width), t);
            }
            for (uint32_t y = this->block_y0(by); y < this->block_y1(by, input.height); y++) {
                const E* in = &input.data[input.offset + y*input.stride];
                U8* out = &output.data[output.offset + y*output.stride];
                if (otsu.down) {
                    for (uint32_t x = 0; x < width; x++)
                        out[x] = static_cast<U8>((int16_t)(U8)in[x] <= threshold[x]);
                } else {
                    for (uint32_t x = 0; x < width; x++)
                        out[x] = static_cast<U8>((int16_t)(U8)in[x] > threshold[x]);
                }
            }
        }
    };

    /// Applies a threshold to an image from the minimum and maximum values in a regular grid across the image. The
    /// threshold of a block is the scaled average of the minimum and maximum in its local 3x3 region. See
    /// ThresholdBlockCommon.
    ///
    ///     statistics(r):
    ///         for each image row in r:  column min and max of every column x
    ///         min[b], max[b] = min and max of b's column values, or NaN if b's first pixel is NaN
    ///     threshold of b:
    ///         min, max = smallest min[] and largest max[] over region(b)
    ///         t = scale*(max + min)/2
    ///         flat = max - min <= texture_threshold()
    ///     out[x] = flat or the comparison with t
    ///
    /// @author Peter Abeles
    template<class E>
    class ThresholdBlockMinMax : public ThresholdBlockCommon<E> {
    public:
        using sum_type = typename TypeInfo<E>::sum_type;

        // if the min and max value's difference is <= to this value then it is considered
        // to be textureless and a default value is used
        double minimumSpread;
        // Integer images compute with a double scale and others with a float scale
        double scale;
        bool down;

        /// @param minimumSpread If the difference between min max is less than or equal to this
        /// value then it is considered textureless. Integer images round it to the
        /// nearest integer. Set to a negative value to disable.
        /// @param requestedBlockWidth About how wide and tall you wish a block to be in pixels.
        ThresholdBlockMinMax( double minimumSpread, const ConfigLength& requestedBlockWidth,
                              bool thresholdFromLocalBlocks, double scale, bool down )
                : ThresholdBlockCommon<E>(requestedBlockWidth, thresholdFromLocalBlocks),
                  minimumSpread(minimumSpread), scale(scale), down(down) {}

    protected:
        /// Unsigned pixels keep 8 or 16 bit thresholds, which compare faster, see threshold_rows()
        static constexpr bool narrow = std::is_integral_v<E> && std::is_unsigned_v<E>;
        /// Type of a pixel's threshold. Signed integers hold any int average in the sum type
        using threshold_type = std::conditional_t<std::is_integral_v<E> && std::is_signed_v<E>, sum_type, E>;

        std::vector<E> column_min, column_max;  // per column over the rows of a row of blocks
        std::vector<E> mins, maxs;              // per block
        std::vector<threshold_type> column;     // threshold per column
        std::vector<U8> textureless;            // 1 for the columns of a textureless block

        static constexpr E highest() {
            return std::numeric_limits<E>::has_infinity ? std::numeric_limits<E>::infinity()
                                                        : std::numeric_limits<E>::max();
        }

        static constexpr E lowest() {
            return std::numeric_limits<E>::has_infinity ? -std::numeric_limits<E>::infinity()
                                                        : std::numeric_limits<E>::lowest();
        }

    public:
        /// Type of the textureless spread: int for integer images and the pixel type otherwise
        using spread_type = typename std::conditional_t<std::is_integral_v<E>, std::make_signed<sum_type>,
                                                         std::common_type<sum_type>>::type;

        /// The textureless spread. Integers: -1 for a negative spread, otherwise rounded half up, then cast to an int
        spread_type texture_threshold() const {
            if constexpr (std::is_integral_v<E>) {
                if (minimumSpread < 0)
                    return -1;
                // Math.round's ties go up. floor(x + 0.5) would round the double below 0.5 up to 1
                double rounded = std::floor(minimumSpread);
                if (minimumSpread - rounded >= 0.5)
                    rounded += 1;
                return java_int(rounded);
            } else {
                return static_cast<spread_type>(minimumSpread);
            }
        }

    protected:
        void begin( uint32_t width ) override {
            column_min.resize(width);
            column_max.resize(width);
            mins.resize(this->blocksX*this->blocksY);
            maxs.resize(this->blocksX*this->blocksY);
            column.resize(width);
            textureless.resize(width);
        }

        void statistics( const Gray<E>& input, uint32_t by ) override {
            const uint32_t width = input.width;
            const uint32_t y0 = this->block_y0(by), y1 = this->block_y1(by, input.height);
            E* cmin = column_min.data();
            E* cmax = column_max.data();
            std::fill(cmin, cmin + width, highest());
            std::fill(cmax, cmax + width, lowest());
            for (uint32_t y = y0; y < y1; y++) {
                const E* row = &input.data[input.offset + y*input.stride];
                for (uint32_t x = 0; x < width; x++) {
                    cmin[x] = row[x] < cmin[x] ? row[x] : cmin[x];
                    cmax[x] = row[x] > cmax[x] ? row[x] : cmax[x];
                }
            }
            for (uint32_t bx = 0; bx < this->blocksX; bx++) {
                const uint32_t x0 = this->block_x0(bx), x1 = this->block_x1(bx, width);
                E min = highest(), max = lowest();
                for (uint32_t x = x0; x < x1; x++) {
                    min = cmin[x] < min ? cmin[x] : min;
                    max = cmax[x] > max ? cmax[x] : max;
                }
                const E first = input.data[input.offset + y0*input.stride + x0];
                if (first != first)
                    min = max = first;
                mins[by*this->blocksX + bx] = min;
                maxs[by*this->blocksX + bx] = max;
            }
        }

        void threshold_rows( const Gray<E>& input, Gray<U8>& output, uint32_t by ) override {
            const uint32_t width = input.width;
            const spread_type textureThreshold = texture_threshold();
            threshold_type* threshold = column.data();
            U8* flat = textureless.data();
            for (uint32_t bx = 0; bx < this->blocksX; bx++) {
                uint32_t bx0, by0, bx1, by1;
                this->region(bx, by, bx0, by0, bx1, by1);
                E min = std::numeric_limits<E>::max();
                E max = std::numeric_limits<E>::lowest();
                for (uint32_t y = by0; y <= by1; y++) {
                    for (uint32_t x = bx0; x <= bx1; x++) {
                        E localMin = mins[y*this->blocksX + x];
                        E localMax = maxs[y*this->blocksX + x];
                        if (localMin < min)
                            min = localMin;
                        if (localMax > max)
                            max = localMax;
                    }
                }
                bool is_flat = max - min <= textureThreshold;
                threshold_type t;
                if constexpr (narrow) {
                    // An average outside the pixel range marks every pixel the same, and where that's 1 the block is
                    // marked like a textureless one. Down compares pixel < average + 1, so an average of -1 fits
                    const int64_t average = java_int(scale*(((int64_t)max + min)/2));
                    const int64_t largest = std::numeric_limits<E>::max();
                    if (down) {
                        is_flat |= average >= largest;
                        t = (E)(std::clamp<int64_t>(average, -1, largest - 1) + 1);
                    } else {
                        is_flat |= average < 0;
                        t = (E)std::clamp<int64_t>(average, 0, largest);
                    }
                } else if constexpr (std::is_integral_v<E>) {
                    t = java_int(scale*(((int64_t)max + min)/2));
                } else {
                    t = static_cast<float>(scale)*(max + min)/2.0f;
                }
                const uint32_t x0 = this->block_x0(bx), x1 = this->block_x1(bx, width);
                std::fill(threshold + x0, threshold + x1, t);
                std::fill(flat + x0, flat + x1, (U8)is_flat);
            }
            for (uint32_t y = this->block_y0(by); y < this->block_y1(by, input.height); y++) {
                const E* in = &input.data[input.offset + y*input.stride];
                U8* out = &output.data[output.offset + y*output.stride];
                if (down) {
                    if constexpr (narrow) {
                        for (uint32_t x = 0; x < width; x++)
                            out[x] = flat[x] | static_cast<U8>(in[x] < threshold[x]);
                    } else {
                        for (uint32_t x = 0; x < width; x++)
                            out[x] = flat[x] | static_cast<U8>(in[x] <= threshold[x]);
                    }
                } else {
                    for (uint32_t x = 0; x < width; x++)
                        out[x] = flat[x] | static_cast<U8>(!(in[x] <= threshold[x]));
                }
            }
        }
    };
}
#endif
