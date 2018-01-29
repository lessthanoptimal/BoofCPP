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
     * For each pixel it applies the logical 'or' operator between two images.
     *
     * @param inputA First input image. Not modified.
     * @param inputB Second input image. Not modified.
     * @param output Output image. Can be same as either input.
     */
    void logicOr( const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output );

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

    /**
     * Interface for convert an input image into a binary image
     * @tparam T Type of input image.
     */
    template<class T>
    class InputToBinary {
    public:
        virtual void process(T input , Gray<U8> output ) = 0;
    };

    /**
	 * Applies a global threshold across the whole image.  If 'down' is true, then pixels with values <=
	 * to 'threshold' are set to 1 and the others set to 0.  If 'down' is false, then pixels with values >=
	 * to 'threshold' are set to 1 and the others set to 0.
     *
     * @tparam T data type of image
     * @param input Input image that is to be thresholded
     * @param threshold The threshold
     * @param down direction of the threshold
     * @param output Storage for binary thresholded image
     */
    template<class T>
    void threshold( const Gray<T> &input , T threshold , bool down , Gray<T> &output ) {
        output.reshape(input.width,input.height);

        if( down ) {
            for( int y = 0; y < input.height; y++ ) {
                int indexIn = input.offset + y*input.stride;
                int indexOut = output.offset + y*output.stride;

                int end = indexIn + input.width;

                for( ; indexIn < end; indexIn++ , indexOut++ ) {
                    output.data[indexOut] = input.data[indexIn] <= threshold;
                }
            }
        } else {
            for( int y = 0; y < input.height; y++ ) {
                int indexIn = input.offset + y*input.stride;
                int indexOut = output.offset + y*output.stride;

                int end = indexIn + input.width;

                for( ; indexIn < end; indexIn++ , indexOut++ ) {
                    output.data[indexOut] = input.data[indexIn] > threshold;
                }
            }
        }
    }


}

#endif