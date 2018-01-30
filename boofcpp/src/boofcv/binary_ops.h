#ifndef BOOFCPP_BINARY_OPS_H
#define BOOFCPP_BINARY_OPS_H

#include "base_types.h"
#include "images_types.h"
#include "config_types.h"
#include "blur_image.h"

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
     * Computes different variants of Otsu. Can be configured to compute the standard version. This allows the user
     * to better handle textureless regions and can further tune it by scaling the threshold up and down.
     *
     * @author Peter Abeles
     */
    class ComputeOtsu {
    public:
        // computed mean and variance
        double threshold;
        double variance;
        // Is the image being thresholded down or up
        bool down;

    private:
        // which Otsu variant to use
        bool useOtsu2;

        // Tuning parameter. 0 = standard Otsu. Greater than 0 will penalize zero texture.
        double tuning;

        // scale factor applied to the threshold. 1.0 = unmodified
        double scale;

    public:
        /**
         *
         * @param useOtsu2 true to use modified otsu. false uses clasical
         * @param tuning Tuning parameter. 0 = standard Otsu. Greater than 0 will penalize zero texture.
         * @param down Is otsu being used to threshold the image up or down
         * @param scale scale factor applied to the threshold. 1.0 = unmodified
         */
        ComputeOtsu( bool useOtsu2, double tuning, bool down, double scale);

        ComputeOtsu( bool useOtsu2 , bool down );

        void compute(const uint32_t *histogram , uint32_t length , uint32_t totalPixels);

    protected:
        void computeOtsu(const uint32_t* histogram , uint32_t length , uint32_t totalPixels );
        void computeOtsu2(const uint32_t* histogram , uint32_t length , uint32_t totalPixels );

    };

    class ThresholdOps
    {
    public:
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
        static void threshold( const Gray<T> &input , T threshold , bool down , Gray<U8> &output ) {
            output.reshape(input.width,input.height);

            if( down ) {
                for( int y = 0; y < input.height; y++ ) {
                    int indexIn = input.offset + y*input.stride;
                    int indexOut = output.offset + y*output.stride;

                    int end = indexIn + input.width;

                    for( ; indexIn < end; indexIn++ , indexOut++ ) {
                        output.data[indexOut] = (U8)(input.data[indexIn] <= threshold);
                    }
                }
            } else {
                for( int y = 0; y < input.height; y++ ) {
                    int indexIn = input.offset + y*input.stride;
                    int indexOut = output.offset + y*output.stride;

                    int end = indexIn + input.width;

                    for( ; indexIn < end; indexIn++ , indexOut++ ) {
                        output.data[indexOut] = (U8)(input.data[indexIn] > threshold);
                    }
                }
            }
        }

        /**
         * Thresholds the image using a locally adaptive threshold that is computed using a local square region centered
         * on each pixel.  The threshold is equal to the average value of the surrounding pixels times the scale.
         * If down is true then b(x,y) = I(x,y) &le; T(x,y) * scale ? 1 : 0.  Otherwise
         * b(x,y) = I(x,y) * scale &gt; T(x,y) ? 0 : 1
         *
         * @param input Input image.
         * @param output (optional) Output binary image.  If null it will be declared internally.
         * @param width Width of square region.
         * @param scale Scale factor used to adjust threshold.  Try 0.95
         * @param down Should it threshold up or down.
         * @param storage1 Storage for intermediate step.
         * @param storage2 Storage for intermediate step.
         */
        template<class T>
        static void localMean( const Gray<T>& input , Gray<T>& output ,
                               const ConfigLength& width , float scale , bool down ,
                               Gray<T>& storage1 , Gray<T>&  storage2 ) {

            output.reshape(input.width, input.height);
            storage1.reshape(input.width, input.height);
            storage2.reshape(input.width, input.height);


            uint32_t radius = (uint32_t)width.computeI(min(input.width,input.height))/2;

            Gray<T>& mean = storage1;

            BlurImage::mean(input,mean,radius,storage2);

            if( down ) {
                for( int y = 0; y < input.height; y++ ) {
                    int indexIn = input.offset + y*input.stride;
                    int indexOut = output.offset + y*output.stride;
                    int indexMean = mean.offset + y*mean.stride;

                    int end = indexIn + input.width;

                    for( ; indexIn < end; indexIn++ , indexOut++, indexMean++ ) {
                        float threshold = mean.data[indexMean]*scale;

                        if( (input.data[indexIn]) <= threshold )
                            output.data[indexOut] = 1;
                        else
                            output.data[indexOut] = 0;
                    }
                }
            } else {
                for( int y = 0; y < input.height; y++ ) {
                    int indexIn = input.offset + y*input.stride;
                    int indexOut = output.offset + y*output.stride;
                    int indexMean = mean.offset + y*mean.stride;

                    int end = indexIn + input.width;

                    for( ; indexIn < end; indexIn++ , indexOut++, indexMean++ ) {
                        float threshold = mean.data[indexMean];

                        if( input.data[indexIn]*scale > threshold )
                            output.data[indexOut] = 1;
                        else
                            output.data[indexOut] = 0;
                    }
                }
            }
        }
    };


    /**
     * Interface for convert an input image into a binary image
     * @tparam T Type of input image.
     */
    template<class T>
    class InputToBinary {
    public:
        typedef typename T::pixel_type pixel_type;

        virtual void process(const T& input , Gray<U8>& output ) = 0;
    };

    template<class T>
    class GlobalFixedBinaryFilter : public InputToBinary<Gray<T>>
    {
    public:
        T threshold;
        bool down;

        GlobalFixedBinaryFilter( T threshold, bool down ) {
            this->threshold = threshold;
            this->down = down;
        }

        void process(const Gray<T>& input , Gray<U8>& output ) override {
            ThresholdOps::threshold(input,threshold,down,output);
        }
    };

    template<class T>
    class LocalMeanBinaryFilter : public InputToBinary<Gray<T>>
    {
    public:
        ConfigLength regionWidth;
        float scale;
        bool down;
        T storage1;
        T storage2;

        LocalMeanBinaryFilter( const ConfigLength& regionWidth, float scale, bool down ) {
            this->regionWidth = regionWidth;
            this->scale = scale;
            this->down = down;
        }

        void process(const Gray<T>& input , Gray<U8>& output ) override {
            ThresholdOps::localMean(input,output,regionWidth,scale,down,storage1,storage2);
        }
    };
}

#endif