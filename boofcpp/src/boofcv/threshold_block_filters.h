#ifndef BOOFCPP_THRESHOLD_BLOCK_FILTERS_H
#define BOOFCPP_THRESHOLD_BLOCK_FILTERS_H

#include <algorithm>

#include "images_types.h"
#include "base_types.h"
#include "config_types.h"
#include "sanity_checks.h"
#include "binary_ops.h"
#include "image_misc_ops.h"

namespace boofcv
{
    /**
     * <p> Computes image statistics in regularly spaced blocks across the image. Then computes an average
     *     of the statistics in a block within a local 3x3 grid region. The average statistics in a local 3x3 grid
     *     region is used to reduce the adverse affects of using a grid.
     * Ideally a local region around each pixel would be used, but this is expensive to compute.  Since a grid is
     * used instead of a pixel local region boundary conditions can be an issue.  For example, consider a black square
     * in the image, if the grid just happens to lie on this black square perfectly then if you look at only a single
     * grid element it will be considered textureless and the edge lost.  This problem isn't an issue if you consder
     * a local 3x3 region of blocks.</p>
     *
     * <p>The size each block in the grid in pixels is adjusted depending on image size.  This is done to minimize
     * "squares" in the upper image boundaries from having many more pixels than other blocks.</p>
     *
     * <p>The block based approach used here was inspired by a high level description found in AprilTags.</p>
     *
     * @tparam T Type of input image.
     * @tparam S Type of image used to store statistics.
     *
     * @author Peter Abeles
     */
    template<class T, class S>
    class ThresholdBlockCommon {
    public: // NOTE: These would be protected but C++ doesn't make it easy to test protected data structures
        // Stores computed block statistics
        S stats;

        // the desired width and height of a block requested by the user
        ConfigLength requestedBlockWidth;

        // the adjusted size to minimize extra pixels near the image upper extreme
        uint32_t blockWidth,blockHeight;

        // Should it use the local 3x3 block region
        bool thresholdFromLocalBlocks;

    public:
        /**
         * Configures the detector
         * @param requestedBlockWidth About how wide and tall you wish a block to be in pixels.
         */
        ThresholdBlockCommon(const ConfigLength& requestedBlockWidth, bool thresholdFromLocalBlocks ) {
            this->requestedBlockWidth = requestedBlockWidth;
            this->thresholdFromLocalBlocks = thresholdFromLocalBlocks;
            this->blockWidth = 0;
            this->blockHeight = 0;
        }

        virtual ~ThresholdBlockCommon() = default;

        /**
         * Converts the gray scale input image into a binary image
         * @param input Input image
         * @param output Output binary image
         */
        void process(const T& input , Gray<U8>& output ) {
            checkSameShape(input,output);

            auto requestedBlockWidth = (uint32_t)this->requestedBlockWidth.computeI( std::min(input.width,input.height));
            if( input.width < requestedBlockWidth || input.height < requestedBlockWidth ) {
                throw invalid_argument("Image is smaller than block size");
            }

            selectBlockSize(input.width,input.height,(uint32_t)requestedBlockWidth);

            this->stats.reshape(input.width/blockWidth,input.height/blockHeight);

            uint32_t innerWidth = input.width%blockWidth == 0 ?
                                input.width : input.width-blockWidth-(input.width%blockWidth);
            uint32_t innerHeight = input.height%blockHeight == 0 ?
                                input.height : input.height-blockHeight-(input.height%blockHeight);

            computeStatistics(input, innerWidth, innerHeight);
            applyThreshold(input,output);
        }

        /**
         * Selects a block size which is close to the requested block size by the user
         */
        void selectBlockSize( uint32_t width , uint32_t height , uint32_t requestedBlockWidth) {

            uint32_t rows = height/requestedBlockWidth;
            uint32_t cols = width/requestedBlockWidth;

            blockHeight = height/rows;
            blockWidth = width/cols;
        }

        /**
         * Applies the dynamically computed threshold to each pixel in the image, one block at a time
         */
        void applyThreshold( const T& input, Gray<U8>& output ) {
            for (uint32_t blockY = 0; blockY < stats.height; blockY++) {
                for (uint32_t blockX = 0; blockX < stats.width; blockX++) {
                    thresholdBlock(blockX,blockY,input,output);
                }
            }
        }

        /**
         * Computes the min-max value for each block in the image
         */
        virtual void computeStatistics( const T& input, uint32_t innerWidth, uint32_t innerHeight) {
            int statPixelStride = stats.num_bands;

            uint32_t indexStats = 0;
            for (uint32_t y = 0; y < innerHeight; y += blockHeight) {
                for (uint32_t x = 0; x < innerWidth; x += blockWidth, indexStats += statPixelStride) {
                    computeBlockStatistics(x,y,blockWidth,blockHeight,indexStats,input);
                }
                // handle the case where the image's width isn't evenly divisible by the block's width
                if( innerWidth != input.width ) {
                    computeBlockStatistics(innerWidth,y,input.width-innerWidth,blockHeight,indexStats,input);
                    indexStats += statPixelStride;
                }
            }
            // handle the case where the image's height isn't evenly divisible by the block's height
            if( innerHeight != input.height ) {
                uint32_t y = innerHeight;
                uint32_t blockHeight = input.height-innerHeight;
                for (uint32_t x = 0; x < innerWidth; x += blockWidth, indexStats += statPixelStride) {
                    computeBlockStatistics(x,y,blockWidth,blockHeight,indexStats,input);
                }
                if( innerWidth != input.width ) {
                    computeBlockStatistics(innerWidth,y,input.width-innerWidth,blockHeight,indexStats,input);
                }
            }
        }

        /**
         * Computes the min-max value inside a block
         * @param x0 lower bound pixel value of block, x-axis
         * @param y0 upper bound pixel value of block, y-axis
         * @param width Block's width
         * @param height Block's height
         * @param indexStats array index of statistics image pixel
         * @param input Input image
         */
        virtual void computeBlockStatistics(uint32_t x0 , uint32_t y0 ,
                                            uint32_t width , uint32_t height ,
                                            uint32_t indexStats , const T& input) = 0;

        /**
         * Thresholds all the pixels inside the specified block
         * @param blockX0 Block x-coordinate
         * @param blockY0 Block y-coordinate
         * @param input Input image
         * @param output Output image
         */
        virtual void thresholdBlock(uint32_t blockX0 , uint32_t blockY0 , const T& input, Gray<U8>& output ) = 0;

        bool isThresholdFromLocalBlocks() {
            return thresholdFromLocalBlocks;
        }

        void setThresholdFromLocalBlocks(bool thresholdFromLocalBlocks) {
            this->thresholdFromLocalBlocks = thresholdFromLocalBlocks;
        }
    };

    /**
     * <p>
     * Applies a threshold to an image by computing the mean values in a regular grid across
     * the image.  When thresholding all the pixels inside a box (grid element) the mean values is found
     * in the surrounding 3x3 grid region.\
     * </p>
     *
     * <p>See {@link ThresholdBlockMinMax} for a more detailed discussion of elements of this strategy</p>
     *
     * @author Peter Abeles
     */
    template<class E>
    class ThresholdBlockMean : public ThresholdBlockCommon<Gray<E>,Interleaved<E>>
    {
    public:
        float scale; // in the java version U8 uses double and F32 uses float. using double causes slight diff in float results
        bool down;

        ThresholdBlockMean(const ConfigLength &requestedBlockWidth, bool thresholdFromLocalBlocks,
                           double scale , bool down)
                : ThresholdBlockCommon<Gray<E>,Interleaved<E>>(requestedBlockWidth, thresholdFromLocalBlocks) {
            this->scale = (float)scale;
            this->down = down;
            this->stats.setNumberOfBands(1);
        }

        virtual ~ThresholdBlockMean() = default;

        void thresholdBlock(uint32_t blockX0 , uint32_t blockY0 , const Gray<E>& input, Gray<U8>& output ) override {

            uint32_t x0 = blockX0*this->blockWidth;
            uint32_t y0 = blockY0*this->blockHeight;

            uint32_t x1 = blockX0==this->stats.width-1 ? input.width : (blockX0+1)*this->blockWidth;
            uint32_t y1 = blockY0==this->stats.height-1 ? input.height: (blockY0+1)*this->blockHeight;

            // define the local 3x3 region in blocks, taking in account the image border
            uint32_t blockX1, blockY1;
            if(this->thresholdFromLocalBlocks) {
                blockX1 = std::min(this->stats.width - 1, blockX0 + 1);
                blockY1 = std::min(this->stats.height - 1, blockY0 + 1);

                blockX0 = blockX0 > 0 ? blockX0 - 1 : 0;
                blockY0 = blockY0 > 0 ? blockY0 - 1 : 0;
            } else {
                blockX1 = blockX0;
                blockY1 = blockY0;
            }

            // Average the mean across local blocks
            typename TypeInfo<E>::sum_type sum = 0;

            for (uint32_t y = blockY0; y <= blockY1; y++) {
                for (uint32_t x = blockX0; x <= blockX1; x++) {
                    sum += this->stats.unsafe_at(x,y,0);
                }
            }
            E mean = sum/((blockY1-blockY0+1)*(blockX1-blockX0+1));

            // apply threshold
            for (uint32_t y = y0; y < y1; y++) {
//                uint32_t indexInput = input.offset + y*input.stride + x0;
//                uint32_t indexOutput = output.offset + y*output.stride + x0;
//                uint32_t end = indexOutput + (x1-x0);
//                for (; indexOutput < end; indexOutput++, indexInput++ ) {
//                    output.data[indexOutput] = (U8)(down == (input.data[indexInput] <= mean));
//                }
                E* inptr = &input.data[input.offset + y*input.stride + x0];
                U8* outptr = &output.data[output.offset + y*output.stride + x0];
                E* end = &inptr[x1-x0];

                while( inptr != end ) {
                    *outptr++ = (U8)(down == (*inptr++ <= mean));
                }
            }
        }

        void computeBlockStatistics(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height, uint32_t indexStats,
                                    const Gray<E>& input) override {
            typedef typename TypeInfo<E>::sum_type sum_type;
            sum_type sum = 0;

            for (uint32_t y = 0; y < height; y++) {
//                int indexInput = input.offset + (y0+y)*input.stride + x0;
//                for (int x = 0; x < width; x++) {
//                    sum += input.data[indexInput++];
//                }
                E* ptr = &input.data[input.offset + (y0+y)*input.stride + x0];
                E* end = &ptr[width];
                while( ptr != end ) {
                    sum += *ptr++;
                }
            }
            if( std::numeric_limits<sum_type>::is_integer )
                sum = static_cast<sum_type>(scale*sum/(width*height)+0.5f);
            else {
                sum = static_cast<sum_type>(scale*sum/(width*height));
            }
            this->stats.data[indexStats] = static_cast<E>(sum);
        }
    };

    /**
     * Block Otsu threshold implementation based on {@link ThresholdBlockCommon}. Computes a histogram in non-overlapping
     * square regions. Then thresholds a single region by combining histograms from its neighbors to make it less blocky.
     *
     * This implementation includes a modification from the traditional Otsu algorithm. The threshold can optionally
     * be adjusted in low variance regions. See code for details.
     *
     * <p>NOTE: This produces visually different results from {@link ThresholdBlockOtsu} because the block algorithm
     * combines histograms from its neighboring blocks. That's why it appears to have a wider effective block.</p>
     *
     * @see GThresholdImageOps#computeOtsu(ImageGray, double, double)
     *
     * @author Peter Abeles
     */
    template<class E>
    class ThresholdBlockOtsu : public ThresholdBlockCommon<Gray<E>,Interleaved<S32>> {
    public:

        GrowArray<uint32_t> histogram;

        ComputeOtsu otsu;

        /**
         * Configures the detector
         *
         * @param requestedBlockWidth About how wide and tall you wish a block to be in pixels.
         * @param tuning Tuning parameter. 0 = standard Otsu. Greater than 0 will penalize zero texture.
         */
         ThresholdBlockOtsu(bool otsu2, ConfigLength requestedBlockWidth, double tuning, double scale, bool down,
                            bool thresholdFromLocalBlocks )
                 : ThresholdBlockCommon<Gray<E>,Interleaved<S32>>(requestedBlockWidth, thresholdFromLocalBlocks),
                   histogram(256) , otsu(otsu2,tuning,down,scale)
        {
            this->stats.setNumberOfBands(256);
        }

        void computeStatistics( const Gray<E>& input, uint32_t innerWidth, uint32_t innerHeight) override {
            ImageMiscOps::fill(this->stats,0);
            ThresholdBlockCommon<Gray<E>,Interleaved<S32>>::computeStatistics(input,innerWidth,innerHeight);
        }

        void computeBlockStatistics(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height, uint32_t indexStats,
                                    const Gray<E>& input) override
        {
            for (uint32_t y = 0; y < height; y++) {
                uint32_t indexInput = input.offset + (y0+y)*input.stride + x0;
                for (uint32_t x = 0; x < width; x++) {
                    this->stats.data[static_cast<uint32_t>(indexStats+input.data[indexInput++])]++;
                }
            }
        }

        void thresholdBlock(uint32_t blockX0 , uint32_t blockY0 , const Gray<E>& input, Gray<U8>& output ) override
        {
            uint32_t x0 = blockX0*this->blockWidth;
            uint32_t y0 = blockY0*this->blockHeight;

            uint32_t x1 = blockX0 == this->stats.width-1  ? input.width : (blockX0+1)*this->blockWidth;
            uint32_t y1 = blockY0 == this->stats.height-1 ? input.height: (blockY0+1)*this->blockHeight;

            // define the local 3x3 region in blocks, taking in account the image border
            uint32_t blockX1, blockY1;
            if(this->thresholdFromLocalBlocks) {
                blockX1 = std::min(this->stats.width  - 1, blockX0 + 1);
                blockY1 = std::min(this->stats.height - 1, blockY0 + 1);

                blockX0 = blockX0 > 0 ? blockX0 - 1 : 0;
                blockY0 = blockY0 > 0 ? blockY0 - 1 : 0;
            } else {
                blockX1 = blockX0;
                blockY1 = blockY0;
            }

            // sum up histogram in local region
            histogram.fill(0);

            for (uint32_t y = blockY0; y <= blockY1; y++) {
                for (uint32_t x = blockX0; x <= blockX1; x++) {
                    int indexStats = this->stats.index_of(x,y,0);
                    for (int i = 0; i < histogram.size; i++) {
                        histogram[i] += this->stats.data[indexStats+i];
                    }
                }
            }

            // this can vary across the image at the borders
            uint32_t total = 0;
            for (int i = 0; i < histogram.size; i++) {
                total += histogram[i];
            }

            // compute threshold
            otsu.compute(histogram,total);

            for (uint32_t y = y0; y < y1; y++) {
                uint32_t indexInput = input.offset + y*input.stride + x0;
                uint32_t indexOutput = output.offset + y*output.stride + x0;
                uint32_t end = indexOutput + (x1-x0);
                for (; indexOutput < end; indexOutput++, indexInput++ ) {
                    output.data[indexOutput] = (U8)(otsu.down == (input.data[indexInput] <= otsu.threshold));
                }
            }
        }
    };
}
#endif