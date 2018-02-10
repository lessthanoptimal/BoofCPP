#ifndef BOOFCPP_CONTOUR_H
#define BOOFCPP_CONTOUR_H

#include <cstdint>
#include "image_types.h"
#include "base_types.h"
#include "packed_sets.h"
#include "geometry_types.h"

namespace boofcv {

    /**
     * List of connectivity rules.
     */
    enum ConnectRule {
        FOUR,EIGHT
    };

    /**
     * Used to trace the external and internal contours around objects for {@link LinearContourLabelChang2004}.  As it
     * is tracing an object it will modify the binary image by labeling.  The input binary image is assumed to have
     * a 1-pixel border that needs to be compensated for.
     *
     * @author Peter Abeles
    */
    class ContourTracer {
    public:

        // Stops saving the contour when it meets or exceeds this value
        uint32_t maxContourSize= std::numeric_limits<uint32_t>::max();

        // which connectivity rule is being used. 4 and 8 supported
        ConnectRule rule;
        uint32_t ruleN;

        // storage for contour points.
        PackedSet<Point2D<S32>>* storagePoints;

        // binary image being traced
        Gray<U8>* binary;
        // label image being marked
        Gray<S32>* labeled;

        // coordinate of pixel being examined (x,y)
        uint32_t x,y;
        // label of the object being traced
        int label;
        // direction it moved in
        uint32_t dir;
        // index of the pixel in the image's internal array
        uint32_t indexBinary;
        uint32_t indexLabel;

        // the pixel index offset to each neighbor
        int32_t *offsetsBinary;
        int32_t *offsetsLabeled;
        // lookup table for which direction it should search next given the direction it traveled into the current pixel
        uint32_t *nextDirection;

        /**
         * Specifies connectivity rule
         *
         * @param rule Specifies 4 or 8 as connectivity rule
         */
        ContourTracer( ConnectRule rule ) :
                storagePoints(nullptr), binary(nullptr), labeled(nullptr)
        {
            this->rule = rule;

            if( ConnectRule::EIGHT == rule ) {
                // start the next search +2 away from the square it came from
                // the square it came from is the opposite from the previous 'dir'
                nextDirection = new uint32_t[8];
                for( uint32_t i = 0; i < 8; i++ )
                    nextDirection[i] = ((i+4)%8 + 2)%8;
                ruleN = 8;
            } else if( ConnectRule::FOUR == rule ) {
                nextDirection = new uint32_t[4];
                for( uint32_t i = 0; i < 4; i++ )
                    nextDirection[i] = ((i+2)%4 + 1)%4;
                ruleN = 4;
            } else {
                throw std::invalid_argument("Connectivity rule must be 4 or 8");
            }

            offsetsBinary = new int32_t[ruleN];
            offsetsLabeled = new int32_t[ruleN];
        }

        ~ContourTracer() {
            delete []offsetsBinary;
            delete []offsetsLabeled;
            delete []nextDirection;

            offsetsBinary = nullptr;
            offsetsLabeled = nullptr;
            nextDirection = nullptr;
        }

        /**
         *
         * @param binary Binary image with a border of zeros added to the outside.
         * @param labeled Labeled image.  Size is the same as the original binary image without border.
         * @param storagePoints
         */
        void setInputs( Gray<U8>& binary , Gray<S32>& labeled , PackedSet<Point2D<S32>>& storagePoints ) {
            this->binary = &binary;
            this->labeled = &labeled;
            this->storagePoints = &storagePoints;

            if( rule == ConnectRule::EIGHT ) {
                setOffsets8(offsetsBinary,binary.stride);
                setOffsets8(offsetsLabeled,labeled.stride);
            } else {
                setOffsets4(offsetsBinary,binary.stride);
                setOffsets4(offsetsLabeled,labeled.stride);
            }
        }

        void setOffsets8( int32_t *offsets , uint32_t stride ) {
            int32_t s = stride;
            offsets[0] =  1;   // x =  1 y =  0
            offsets[1] =  1+s; // x =  1 y =  1
            offsets[2] =    s; // x =  0 y =  1
            offsets[3] = -1+s; // x = -1 y =  1
            offsets[4] = -1  ; // x = -1 y =  0
            offsets[5] = -1-s; // x = -1 y = -1
            offsets[6] =   -s; // x =  0 y = -1
            offsets[7] =  1-s; // x =  1 y = -1
        }

        void setOffsets4( int32_t *offsets , uint32_t stride ) {
            int32_t s = stride;
            offsets[0] =  1;   // x =  1 y =  0
            offsets[1] =    s; // x =  0 y =  1
            offsets[2] = -1;   // x = -1 y =  0
            offsets[3] =   -s; // x =  0 y = -1
        }

        /**
         *
         * @param label
         * @param initialX
         * @param initialY
         * @param external True for tracing an external contour or false for internal.
         */
        void trace( int label , uint32_t initialX , uint32_t initialY , bool external )
        {
            uint32_t initialDir;
            if( rule == ConnectRule::EIGHT )
                initialDir = external ? 7 : 3;
            else
                initialDir = external ? 0 : 2;

            this->label = label;
            this->dir = initialDir;
            x = initialX;
            y = initialY;

            // index of pixels in the image array
            // binary has a 1 pixel border which labeled lacks, hence the -1,-1 for labeled
            indexBinary = binary->index_of(x,y);
            indexLabel = labeled->index_of(x-1,y-1);
            add(x,y);

            // find the next black pixel.  handle case where its an isolated point
            if( !searchBlack() ) {
                return;
            } else {
                initialDir = dir;
                moveToNext();
                dir = nextDirection[dir];
            }

            for( ;; ) {
                // search in clockwise direction around the current pixel for next black pixel
                searchBlack();
                if( x == initialX && y == initialY && dir == initialDir ) {
                    // returned to the initial state again. search is finished
                    return;
                } else {
                    add(x, y);
                    moveToNext();
                    dir = nextDirection[dir];
                }
            }
        }

        /**
         * Searches in a circle around the current point in a clock-wise direction for the first black pixel.
         */
        bool searchBlack() {
//		for( int i = 0; i < ruleN; i++ ) {
//			if( checkBlack(indexBinary + offsetsBinary[dir]))
//				return true;
//			dir = (dir+1)%ruleN;
//		}
//		return false;

            // Unrolling here results in about a 10% speed up
            if( ruleN == 4 )
                return searchBlack4();
            else
                return searchBlack8();
        }

        bool searchBlack4() {
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%4;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%4;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%4;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%4;
            return false;
        }

        bool searchBlack8() {
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            if( checkBlack(indexBinary + offsetsBinary[dir]))
                return true;
            dir = (dir+1)%8;
            return false;
        }

        /**
         * Checks to see if the specified pixel is black (1).  If not the pixel is marked so that it
         * won't be searched again
         */
        bool checkBlack( uint32_t index ) {
            if( binary->data[index] == 1 ) {
                return true;
            } else {
                // mark white pixels to avoid retracing this contour in the future
                binary->data[index] = 255;
                return false;
            }
        }

        void moveToNext() {
            // move to the next pixel using the precomputed pixel index offsets
            indexBinary += offsetsBinary[dir];
            indexLabel += offsetsLabeled[dir];
            // compute the new pixel coordinate from the binary pixel index
            int a = indexBinary - binary->offset;
            x = a%binary->stride;
            y = a/binary->stride;
        }

        /**
         * Adds a point to the contour list
         */
        void add( int x , int y ) {
            labeled->data[indexLabel] = label;
            if( storagePoints->size_of_tail() < maxContourSize ) {
                storagePoints->push_into_tail( Point2D<S32>(x - 1, y - 1));
            }
        }

        void setMaxContourSize(uint32_t maxContourSize) {
            this->maxContourSize = maxContourSize;
        }

        ConnectRule getConnectRule() {
            return rule;
        }
    };
}

#endif