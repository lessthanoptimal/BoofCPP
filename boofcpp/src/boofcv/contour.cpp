#include <cmath>
#include <stdexcept>
#include "contour.h"

using namespace boofcv;

/**
 * Specifies connectivity rule
 *
 * @param rule Specifies 4 or 8 as connectivity rule
 */
ContourTracer::ContourTracer(ConnectRule rule) :
        storagePoints(nullptr), binary(nullptr), labeled(nullptr) {
    this->rule = rule;

    if (ConnectRule::EIGHT == rule) {
        // start the next search +2 away from the square it came from
        // the square it came from is the opposite from the previous 'dir'
        nextDirection = new uint32_t[8];
        for (uint32_t i = 0; i < 8; i++)
            nextDirection[i] = ((i + 4) % 8 + 2) % 8;
        ruleN = 8;
    } else if (ConnectRule::FOUR == rule) {
        nextDirection = new uint32_t[4];
        for (uint32_t i = 0; i < 4; i++)
            nextDirection[i] = ((i + 2) % 4 + 1) % 4;
        ruleN = 4;
    } else {
        throw std::invalid_argument("Connectivity rule must be 4 or 8");
    }

    offsetsBinary = new int32_t[ruleN];
    offsetsLabeled = new int32_t[ruleN];
}

ContourTracer::~ContourTracer() {
    delete[]offsetsBinary;
    delete[]offsetsLabeled;
    delete[]nextDirection;

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
void ContourTracer::set_inputs(Gray<U8> &binary, Gray<S32> &labeled, PackedSet<Point2D<S32>> &storagePoints) {
    this->binary = &binary;
    this->labeled = &labeled;
    this->storagePoints = &storagePoints;

    if (rule == ConnectRule::EIGHT) {
        setOffsets8(offsetsBinary, binary.stride);
        setOffsets8(offsetsLabeled, labeled.stride);
    } else {
        setOffsets4(offsetsBinary, binary.stride);
        setOffsets4(offsetsLabeled, labeled.stride);
    }
}

void ContourTracer::setOffsets8(int32_t *offsets, uint32_t stride) {
    int32_t s = stride;
    offsets[0] = 1;      // x =  1 y =  0
    offsets[1] = 1 + s;  // x =  1 y =  1
    offsets[2] = s;      // x =  0 y =  1
    offsets[3] = -1 + s; // x = -1 y =  1
    offsets[4] = -1;     // x = -1 y =  0
    offsets[5] = -1 - s; // x = -1 y = -1
    offsets[6] = -s;     // x =  0 y = -1
    offsets[7] = 1 - s;  // x =  1 y = -1
}

void ContourTracer::setOffsets4(int32_t *offsets, uint32_t stride) {
    int32_t s = stride;
    offsets[0] = 1;   // x =  1 y =  0
    offsets[1] = s;   // x =  0 y =  1
    offsets[2] = -1;  // x = -1 y =  0
    offsets[3] = -s;  // x =  0 y = -1
}

/**
 *
 * @param label
 * @param initialX
 * @param initialY
 * @param external True for tracing an external contour or false for internal.
 */
void ContourTracer::trace(int label, uint32_t initialX, uint32_t initialY, bool external) {
    uint32_t initialDir;
    if (rule == ConnectRule::EIGHT)
        initialDir = external ? 7 : 3;
    else
        initialDir = external ? 0 : 2;

    this->label = label;
    this->dir = initialDir;
    x = initialX;
    y = initialY;

    // index of pixels in the image array
    // binary has a 1 pixel border which labeled lacks, hence the -1,-1 for labeled
    indexBinary = binary->index_of(x, y);
    indexLabel = labeled->index_of(x - 1, y - 1);
    add(x, y);

    // find the next black pixel.  handle case where its an isolated point
    if (!searchBlack()) {
        return;
    } else {
        initialDir = dir;
        moveToNext();
        dir = nextDirection[dir];
    }

    for (;;) {
        // search in clockwise direction around the current pixel for next black pixel
        searchBlack();
        if (x == initialX && y == initialY && dir == initialDir) {
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
bool ContourTracer::searchBlack() {
//		for( int i = 0; i < ruleN; i++ ) {
//			if( checkBlack(indexBinary + offsetsBinary[dir]))
//				return true;
//			dir = (dir+1)%ruleN;
//		}
//		return false;

    // Unrolling here results in about a 10% speed up
    if (ruleN == 4)
        return searchBlack4();
    else
        return searchBlack8();
}

bool ContourTracer::searchBlack4() {
    uint8_t *binary_ptr = &binary->data[indexBinary];

    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 4;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 4;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 4;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 4;
    return false;
}

bool ContourTracer::searchBlack8() {
    uint8_t *binary_ptr = &binary->data[indexBinary];

    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    if (checkBlack(binary_ptr[offsetsBinary[dir]]))
        return true;
    dir = (dir + 1) % 8;
    return false;
}

/**
 * Checks to see if the specified pixel is black (1).  If not the pixel is marked so that it
 * won't be searched again
 */
bool ContourTracer::checkBlack(uint8_t &pixel) {
    if (pixel == 1) {
        return true;
    } else {
        // mark white pixels to avoid retracing this contour in the future
        pixel = 255;
        return false;
    }
}

void ContourTracer::moveToNext() {
    // move to the next pixel using the precomputed pixel index offsets
    indexBinary += offsetsBinary[dir];
    indexLabel += offsetsLabeled[dir];
    // compute the new pixel coordinate from the binary pixel index
    uint32_t a = indexBinary - binary->offset;
    x = a % binary->stride;
    y = a / binary->stride;
}

/**
 * Adds a point to the contour list
 */
void ContourTracer::add(int x, int y) {
    labeled->data[indexLabel] = label;
    if (storagePoints->size_of_tail() < maxContourSize) {
        storagePoints->push_tail(Point2D<S32>(x - 1, y - 1));
    }
}

void ContourTracer::setMaxContourSize(uint32_t maxContourSize) {
    this->maxContourSize = maxContourSize;
}

ConnectRule ContourTracer::getConnectRule() {
    return rule;
}

ContourPacked::ContourPacked() :
        id(std::numeric_limits<uint32_t>::max()),
        externalIndex(std::numeric_limits<uint32_t>::max())
{

}

void ContourPacked::reset() {
    id = std::numeric_limits<uint32_t>::max();
    externalIndex = std::numeric_limits<uint32_t>::max();
    internalIndexes.clear();
}

LinearContourLabelChang2004::LinearContourLabelChang2004( ConnectRule rule ) :
        rule(rule) ,
        tracer4(ConnectRule::FOUR) ,
        tracer8(ConnectRule::EIGHT) ,
        packedPoints(2000)
{
}

/**
 * Processes the binary image to find the contour of and label blobs.
 *
 * @param binary Input binary image. Not modified.
 * @param labeled Output. Labeled image.  Modified.
 */
void LinearContourLabelChang2004::process( const Gray<U8>& binary , Gray<S32>& labeled ) {
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
void LinearContourLabelChang2004::handleStep1() {
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
void LinearContourLabelChang2004::handleStep2(Gray<S32>& labeled, uint32_t label) {
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
void LinearContourLabelChang2004::handleStep3(Gray<S32>& labeled) {
    if( labeled.data[indexOut] == 0 )
        labeled.data[indexOut] = labeled.data[indexOut-1];
}

void LinearContourLabelChang2004::setConnectRule( ConnectRule rule ) {
    this->rule = rule;
}

ConnectRule LinearContourLabelChang2004::getConnectRule() {
    return rule;
}