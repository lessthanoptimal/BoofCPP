#ifndef BOOFCPP_SANITY_CHECKS_H
#define BOOFCPP_SANITY_CHECKS_H

#include "base_types.h"
#include "image_types.h"

namespace boofcv
{
    /**
     * Checks to see if the two images have the same shape. If not an exception is thrown
     */
    void checkSameShape( const ImageBase& a , const ImageBase& b );

    /**
     * Checks to see if the three images have the same shape. If not an exception is thrown
     */
    void checkSameShape( const ImageBase& a , const ImageBase& b , const ImageBase& c );
}

#endif