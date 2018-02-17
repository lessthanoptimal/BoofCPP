#ifndef BOOFCPP_CONTOUR_H
#define BOOFCPP_CONTOUR_H

#include <cstdint>
#include "image_types.h"
#include "base_types.h"
#include "packed_sets.h"
#include "geometry_types.h"
#include "image_misc_ops.h"

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
        uint32_t maxContourSize = std::numeric_limits<uint32_t>::max();

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
        void set_inputs( Gray<U8>& binary , Gray<S32>& labeled , PackedSet<Point2D<S32>>& storagePoints ) {
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
                storagePoints->push_tail( Point2D<S32>(x - 1, y - 1));
            }
        }

        void setMaxContourSize(uint32_t maxContourSize) {
            this->maxContourSize = maxContourSize;
        }

        ConnectRule getConnectRule() {
            return rule;
        }
    };

    /**
     * Internal and externals contours for a binary blob with the actual points stored in a
     * {@link boofcv.struct.PackedSetsPoint2D_I32}.  The set of points in each contour list are ordered in
     * CW or CCW directions.
     *
     * @author Peter Abeles
     */
    class ContourPacked {
    public:
        /**
         * ID of blob in the image.  Pixels belonging to this blob in the labeled image will have this pixel value.
         */
        uint32_t id;

        /**
         * Index in the packed list of the external contour
         */
        uint32_t externalIndex;
        /**
         * Number of internal contours. Their ID = external + 1 + internal index
         */
        std::vector<uint32_t> internalIndexes;

        ContourPacked() :
                id(std::numeric_limits<uint32_t>::max()),
                externalIndex(std::numeric_limits<uint32_t>::max())
        {

        }

        void reset() {
            id = std::numeric_limits<uint32_t>::max();
            externalIndex = std::numeric_limits<uint32_t>::max();
            internalIndexes.clear();
        }
    };

    /**
     * <p>
     * Finds objects in a binary image by tracing their contours.  The output is labeled binary image, set of external
     * and internal contours for each object/blob.  Blobs can be defined using a 4 or 8 connect rule.  The algorithm
     * works by processing the image in a single pass.  When a new object is encountered its contour is traced.  Then
     * the inner pixels are labeled.  If an internal contour is found it will also be traced.  See [1] for all
     * the details.  The original algorithm has been modified to use different connectivity rules.
     * </p>
     *
     * <p>
     * Output: Background pixels (0 in input image) are assigned a value of 0, Each blob is then assigned a unique
     * ID starting from 1 up to the number of blobs.
     * </p>
     *
     * <p>The maximum contour size says how many elements are allowed in a contour. if that number is exceeded
     * by the external contour the external contour will be zeroed. Internal contours that exceed it will
     * be discarded. Note that the all external and internal contours will still be traversed they will
     * just not be recorded if too large.</p>
     *
     * <p>
     * Internally, the input binary image is copied into another image which will have a 1 pixel border of all zeros
     * around it.  This ensures that boundary checks will not need to be done, speeding up the algorithm by about 25%.
     * </p>
     *
     * <p>
     * [1] Fu Chang and Chun-jen Chen and Chi-jen Lu, "A linear-time component-labeling algorithm using contour
     * tracing technique" Computer Vision and Image Understanding, 2004
     * </p>
     *
     * @author Peter Abeles
     */
    class LinearContourLabelChang2004 {
    public:

        // The maximum number of elements in a contour that will be recorded
        uint32_t minContourSize = 0;
        // The maximum number of elements in a contour that will be recorded
        uint32_t maxContourSize = std::numeric_limits<uint32_t>::max();
        // If false it will not save internal contours as they are found
        bool saveInternalContours = true;

        // traces edge pixels
        ContourTracer tracer4;
        ContourTracer tracer8;
        ContourTracer *tracer;
        // which rule is active
        ConnectRule rule;

        // binary image with a border of zero.
        Gray<U8> border;

        // predeclared/recycled data structures
        PackedSet<Point2D<S32>> packedPoints;
        std::vector<ContourPacked> contours;

        // internal book keeping variables
        uint32_t x,y,indexIn,indexOut;

        /**
         * Configures the algorithm.
         *
         * @param rule Connectivity rule.  4 or 8
         */
        LinearContourLabelChang2004( ConnectRule rule ) :
                rule(rule) ,
                tracer4(ConnectRule::FOUR) ,
                tracer8(ConnectRule::EIGHT) ,
                packedPoints(2000){
        }

        /**
         * Processes the binary image to find the contour of and label blobs.
         *
         * @param binary Input binary image. Not modified.
         * @param labeled Output. Labeled image.  Modified.
         */
        void process( const Gray<U8>& binary , Gray<S32>& labeled ) {
            // initialize data structures
            labeled.reshape(binary.width,binary.height);

            // ensure that the image border pixels are filled with zero by enlarging the image
            if( border.width != binary.width+2 || border.height != binary.height+2)  {
                border.reshape(binary.width + 2, binary.height + 2);
                ImageMiscOps::fill_border(border, (U8)0, 1);
            }
            border.makeSubimage(1,1,border.width-1,border.height-1).copy(binary);

            // labeled image must initially be filled with zeros
            ImageMiscOps::fill(labeled,(S32)0);

            packedPoints.clear();
            contours.clear();

            tracer = rule == ConnectRule::FOUR ? &tracer4 : &tracer8;

            tracer->set_inputs(border,labeled, packedPoints);

            // Outside border is all zeros so it can be ignored
            uint32_t endY = border.height-1, enxX = border.width-1;
            for( y = 1; y < endY; y++ ) {
                indexIn = border.offset + y*border.stride+1;
                indexOut = labeled.offset + (y-1)*labeled.stride;

                for( x = 1; x < enxX; x++ , indexIn++ , indexOut++) {
                    U8 bit = border.data[indexIn];

                    // white pixels are ignored
                    if( bit != 1 )
                        continue;

                    auto label = static_cast<uint32_t>(labeled.data[indexOut]);
                    bool handled = false;
                    if( label == 0 && border.data[indexIn - border.stride ] != 1 ) {
                        handleStep1();
                        handled = true;
                        label = static_cast<uint32_t>(contours.size());
                    }
                    // could be an external and internal contour
                    if( border.data[indexIn + border.stride ] == 0 ) {
                        handleStep2(labeled, label);
                        handled = true;
                    }
                    if( !handled ) {
                        handleStep3(labeled);
                    }
                }
            }
        }

        /**
         *  Step 1: If the pixel is unlabeled and the pixel above is white, then it
         *          must be an external contour of a newly encountered blob.
         */
        void handleStep1() {
            // grow the number of contours by 1
            contours.push_back(ContourPacked());
            ContourPacked &c = contours.back();
            c.id = static_cast<uint32_t>(contours.size());
            tracer->setMaxContourSize(maxContourSize);
            // save the set index for this contour and declare memory for it
            c.externalIndex = packedPoints.number_of_sets();
            packedPoints.start_new_set();
            c.internalIndexes.clear();
            tracer->trace(c.id,x,y,true);

            // Keep track that this was a contour, but free up all the points used in defining it
            if( packedPoints.size_of_tail() >= maxContourSize || packedPoints.size_of_tail() < minContourSize ) {
                packedPoints.remove_tail();
                packedPoints.start_new_set();
            }
        }

        /**
         * Step 2: If the pixel below is unmarked and white then it must be an internal contour
         *         Same behavior it the pixel in question has been labeled or not already
         */
        void handleStep2(Gray<S32>& labeled, uint32_t label) {
            // if the blob is not labeled and in this state it cannot be against the left side of the image
            if( label == 0 )
                label = static_cast<uint32_t>(labeled.data[indexOut-1]);

            ContourPacked& c = contours.at(label-1);
            c.internalIndexes.push_back( (uint32_t)packedPoints.set_info.size() );
            packedPoints.start_new_set();
            tracer->setMaxContourSize(saveInternalContours?maxContourSize:0);
            tracer->trace(label,x,y,false);

            // See if the inner contour exceeded the maximum  or minimum size. If so free its points
            if( packedPoints.size_of_tail() >= maxContourSize || packedPoints.size_of_tail() < minContourSize ) {
                packedPoints.remove_tail();
                packedPoints.start_new_set();
            }
        }

        /**
         * Step 3: Must not be part of the contour but an inner pixel and the pixel to the left must be
         *         labeled
         */
        void handleStep3(Gray<S32>& labeled) {
            if( labeled.data[indexOut] == 0 )
                labeled.data[indexOut] = labeled.data[indexOut-1];
        }

        void setConnectRule( ConnectRule rule ) {
            this->rule = rule;
        }

        ConnectRule getConnectRule() {
            return rule;
        }
    };
}

#endif