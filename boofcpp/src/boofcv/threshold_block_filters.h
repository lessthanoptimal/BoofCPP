#ifndef BOOFCPP_THRESHOLD_BLOCK_FILTERS_H
#define BOOFCPP_THRESHOLD_BLOCK_FILTERS_H

#include "images_types.h"
#include "config_types.h"
#include "sanity_checks.h"

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

        /**
         * Converts the gray scale input image into a binary image
         * @param input Input image
         * @param output Output binary image
         */
        void process(const T& input , Gray<U8>& output ) {
            checkSameShape(input,output);

            int requestedBlockWidth = this->requestedBlockWidth.computeI( std::min(input.width,input.height));
            if( input.width < requestedBlockWidth || input.height < requestedBlockWidth ) {
                throw invalid_argument("Image is smaller than block size");
            }

            selectBlockSize(input.width,input.height,(uint32_t)requestedBlockWidth);

            stats.reshape(input.width/blockWidth,input.height/blockHeight);

            uint32_t innerWidth = input.width%blockWidth == 0 ?
                                input.width : input.width-blockWidth-input.width%blockWidth;
            uint32_t innerHeight = input.height%blockHeight == 0 ?
                                input.height : input.height-blockHeight-input.height%blockHeight;

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
        void computeStatistics( const T& input, uint32_t innerWidth, uint32_t innerHeight) {
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
                                            uint32_t indexStats , T input) = 0;

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
}
#endif