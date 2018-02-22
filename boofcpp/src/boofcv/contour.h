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
        ContourTracer( ConnectRule rule );

        ~ContourTracer();

        /**
         *
         * @param binary Binary image with a border of zeros added to the outside.
         * @param labeled Labeled image.  Size is the same as the original binary image without border.
         * @param storagePoints
         */
        void set_inputs( Gray<U8>& binary , Gray<S32>& labeled , PackedSet<Point2D<S32>>& storagePoints );

        void setOffsets8( int32_t *offsets , uint32_t stride );

        void setOffsets4( int32_t *offsets , uint32_t stride );

        /**
         *
         * @param label
         * @param initialX
         * @param initialY
         * @param external True for tracing an external contour or false for internal.
         */
        void trace( int label , uint32_t initialX , uint32_t initialY , bool external );

        /**
         * Searches in a circle around the current point in a clock-wise direction for the first black pixel.
         */
        bool searchBlack();

        bool searchBlack4();

        bool searchBlack8();

        /**
         * Checks to see if the specified pixel is black (1).  If not the pixel is marked so that it
         * won't be searched again
         */
        bool checkBlack( uint8_t& pixel );

        void moveToNext();

        /**
         * Adds a point to the contour list
         */
        void add( int x , int y );

        void setMaxContourSize(uint32_t maxContourSize);

        ConnectRule getConnectRule();
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

        ContourPacked();

        void reset();
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
        LinearContourLabelChang2004( ConnectRule rule );

        /**
         * Processes the binary image to find the contour of and label blobs.
         *
         * @param binary Input binary image. Not modified.
         * @param labeled Output. Labeled image.  Modified.
         */
        void process( const Gray<U8>& binary , Gray<S32>& labeled );

        /**
         *  Step 1: If the pixel is unlabeled and the pixel above is white, then it
         *          must be an external contour of a newly encountered blob.
         */
        void handleStep1();

        /**
         * Step 2: If the pixel below is unmarked and white then it must be an internal contour
         *         Same behavior it the pixel in question has been labeled or not already
         */
        void handleStep2(Gray<S32>& labeled, uint32_t label);

        /**
         * Step 3: Must not be part of the contour but an inner pixel and the pixel to the left must be
         *         labeled
         */
        void handleStep3(Gray<S32>& labeled);

        void setConnectRule( ConnectRule rule );

        ConnectRule getConnectRule();
    };
}

#endif