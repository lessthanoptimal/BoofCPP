#ifndef BOOFCPP_BINARY_OPS_H
#define BOOFCPP_BINARY_OPS_H

#include "base_types.h"
#include "images_types.h"

namespace boofcv
{
    /**
	 * For each pixel it applies the logical 'and' operator between two images.
	 *
	 * @param inputA First input image. Not modified.
	 * @param inputB Second input image. Not modified.
	 * @param output Output image. Can be same as either input.
     */
    void logicAnd( const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output );

    /**
     * Erodes an image according to a 4-neighborhood.  Unless a pixel is connected to all its neighbors its value
     * is set to zero.
     *
     * @param input  Input image. Not modified.
     * @param output The output image.
     */
    void erode4(const Gray<U8>& input, Gray<U8>& output);

    /**
     * Dilates an image according to a 4-neighborhood.  If a pixel is connected to any other pixel then its output
     * value will be one.
     *
     * @param input  Input image. Not modified.
     * @param output The output image.
     */
    void dilate4(const Gray<U8>& input, Gray<U8>& output);
}

#endif