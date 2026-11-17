#ifndef BOOFCPP_IMAGE_CONVERT_H
#define BOOFCPP_IMAGE_CONVERT_H

#include "boofcv/image_types.h"
#include "boofcv/sanity_checks.h"
#include "boofcv/base_types.h"

namespace boofcv {
    /**
     * Converts the colored image into grayscale by averaging all of its bands
     *
     * @tparam E
     * @param input
     * @param output
     */
    template< typename E>
    void convert_average( const Interleaved<E>& input , Gray<E>& output )
    {
        typedef typename TypeInfo<E>::sum_type sum_type;

        output.reshape(input.width,input.height);

        for( uint32_t y = 0; y < input.height; y++ ) {

            E* input_ptr = &input.data[input.offset + y*input.stride];
            E* output_ptr = &output.data[output.offset + y*output.stride];

            for( uint32_t x = 0; x < input.width; x++ ) {
                sum_type sum = 0;

                for( uint32_t band = 0; band < input.num_bands; band++ ) {
                    sum += *input_ptr++;
                }

                *output_ptr++ = static_cast<E>(sum/input.num_bands);
            }
        }
    }

    /// Converts the pixel type of a gray image by casting each pixel. Same as BoofCV's ConvertImage.convert()
    template<typename I, typename O>
    void convert( const Gray<I>& input, Gray<O>& output ) {
        output.reshape(input.width, input.height);
        for (uint32_t y = 0; y < input.height; y++) {
            const I* in = input.data + input.offset + y*input.stride;
            O* out = output.data + output.offset + y*output.stride;
            for (uint32_t x = 0; x < input.width; x++)
                out[x] = static_cast<O>(in[x]);
        }
    }
}

#endif