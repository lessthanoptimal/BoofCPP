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
}

#endif