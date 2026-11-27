#ifndef BOOFCPP_BINARY_OPS_H
#define BOOFCPP_BINARY_OPS_H

#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include "boofcv/image_misc_ops.h"
#include "boofcv/base_types.h"
#include "boofcv/image_types.h"
#include "boofcv/config_types.h"
#include "boofcv/image_statistics.h"
#include "boofcv/image_blur.h"

namespace boofcv
{
    /// Java's (int) cast of a double: truncates, saturates, and NaN becomes 0. A C++ cast of a value outside int's
    /// range is undefined
    inline int32_t java_int( double value ) {
        using limits = std::numeric_limits<int32_t>;
        if (!(value == value))
            return 0;
        if (value <= (double)limits::min())
            return limits::min();
        if (value >= (double)limits::max())
            return limits::max();
        return static_cast<int32_t>(value);
    }

    class BinaryImageOps {
    public:
        /**
         * For each pixel it applies the logical 'and' operator between two images.
         *
         * @param inputA First input image. Not modified.
         * @param inputB Second input image. Not modified.
         * @param output Output image. Can be same as either input.
         */
        static void logicAnd(const Gray<U8> &inputA, const Gray<U8> &inputB, Gray<U8> &output);

        /**
         * For each pixel it applies the logical 'or' operator between two images.
         *
         * @param inputA First input image. Not modified.
         * @param inputB Second input image. Not modified.
         * @param output Output image. Can be same as either input.
         */
        static void logicOr(const Gray<U8> &inputA, const Gray<U8> &inputB, Gray<U8> &output);

        /**
         * Erodes an image according to a 4-neighborhood.  Unless a pixel is connected to all its neighbors its value
         * is set to zero.
         *
         * @param input  Input image. Not modified.
         * @param output The output image.
         */
        static void erode4(const Gray<U8> &input, Gray<U8> &output);

        /**
         * Dilates an image according to a 4-neighborhood.  If a pixel is connected to any other pixel then its output
         * value will be one.
         *
         * @param input  Input image. Not modified.
         * @param output The output image.
         */
        static void dilate4(const Gray<U8> &input, Gray<U8> &output);
    };


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

        void compute(const GrowArray<uint32_t>& histogram , uint32_t totalPixels);

    protected:
        void computeOtsu(const GrowArray<uint32_t>& histogram , uint32_t totalPixels );
        void computeOtsu2(const GrowArray<uint32_t>&  histogram  , uint32_t totalPixels );

    };

    /// Same results as ComputeOtsu, but only the histogram's non-empty bins are visited. An empty bin leaves
    /// the background and foreground weights and sums unchanged, so its between class variance equals the one before
    /// it and can't be a new maximum
    class ComputeOtsuNonEmpty {
    public:
        double threshold = 0;
        double variance = 0;
        bool down;

        /// See ComputeOtsu
        ComputeOtsuNonEmpty( bool useOtsu2, double tuning, bool down, double scale )
                : down(down), useOtsu2(useOtsu2), tuning(tuning), scale(scale) {}

        /// @param bins Index of each non-empty bin, in increasing order
        /// @param counts Number of pixels in each non-empty bin
        /// @param n Number of non-empty bins
        /// @param length Number of bins in the histogram
        /// @param totalPixels Sum of counts
        void compute( const uint32_t* bins, const uint32_t* counts, uint32_t n, uint32_t length,
                      uint32_t totalPixels );

    private:
        bool useOtsu2;
        double tuning;
        double scale;
    };

    class ThresholdOps
    {
    public:

        /// Threshold from Otsu's method. BoofCV's GThresholdImageOps.computeOtsu(). The histogram has
        /// (int)(1 + max_value - min_value) bins and starts at (int)min_value for integer images, or (float)min_value
        /// for F32.
        ///
        /// @param input Input gray-scale image
        /// @param min_value The minimum value of a pixel in the image (inclusive)
        /// @param max_value The maximum value of a pixel in the image (inclusive)
        /// @param otsu2 False for standard Otsu. True for the variant described in ComputeOtsu
        /// @return Selected threshold, as a histogram index
        /// @tparam Statistics Computes the histogram, ImageStatistics or ImageStatistics_MT
        template<class T, class Statistics = ImageStatistics>
        static uint32_t computeOtsu( const Gray<T>& input , double min_value , double max_value , bool otsu2=false ) {
            // The histogram needs 1 to INT32_MAX bins. C++'s casts of other values would be undefined
            const double range = std::trunc(1 + max_value - min_value);
            if (!(range >= 1 && range <= std::numeric_limits<int32_t>::max()))
                throw std::invalid_argument("Bad histogram range");
            GrowArray<uint32_t> histogram((uint32_t)range);

            if constexpr (std::is_integral_v<T>) {
                const double min_int = std::trunc(min_value);
                if (!(min_int >= std::numeric_limits<int32_t>::min() && min_int <= std::numeric_limits<int32_t>::max()))
                    throw std::invalid_argument("min_value doesn't fit an int");
                Statistics::histogram(input,(int32_t)min_int,histogram);
            } else {
                Statistics::histogram(input,(T)min_value,histogram);
            }

            // this should configure so that all the really fancy stuff has been turned off
            ComputeOtsu otsu(otsu2,0,true,1.0);
            otsu.compute(histogram, input.total_pixels());
            return (uint32_t)otsu.threshold;
        }

        /**
          * Applies a global threshold across the whole image.  If 'down' is true, then pixels with values <=
          * to 'threshold' are set to 1 and the others set to 0.  If 'down' is false, then pixels with values >=
          * to 'threshold' are set to 1 and the others set to 0.
          *
          * @tparam T data type of image
          * @tparam TH data type of the threshold. Can be wider than T, e.g. int for U8 images, so that a threshold
          *            outside of the pixel range doesn't wrap.
          * @param input Input image that is to be thresholded
          * @param threshold The threshold
          * @param down direction of the threshold
          * @param output Storage for binary thresholded image
          */
        template<class T, class TH = T>
        static void threshold( const Gray<T> &input , TH threshold , bool down , Gray<U8> &output ) {
            output.reshape(input.width,input.height);

            if( down ) {
                for( uint32_t y = 0; y < input.height; y++ ) {
                    T* input_ptr = &input.data[input.offset + y*input.stride];
                    U8* output_ptr = &output.data[output.offset + y*output.stride];

                    for( uint32_t i = input.width; i; i-- ) {
                        *output_ptr++ = static_cast<U8>( *input_ptr++ <= threshold );
                    }
                }
            } else {
                for( uint32_t y = 0; y < input.height; y++ ) {
                    T* input_ptr = &input.data[input.offset + y*input.stride];
                    U8* output_ptr = &output.data[output.offset + y*output.stride];

                    for( uint32_t i = input.width; i; i-- ) {
                        *output_ptr++ = static_cast<U8>( *input_ptr++ > threshold );
                    }
                }
            }
        }

        /**
         * Applies a global threshold specified as a double, the same way BoofCV's GThresholdImageOps.threshold() does.
         * Integer images: threshold is cast to int. Values outside of the pixel range are handled without wrapping,
         * while the inner loop stays in the image's data type so it can be vectorized.
         * Floating point images: threshold is cast to float.
         */
        template<class T>
        static void threshold_double( const Gray<T> &input , double threshold , bool down , Gray<U8> &output ) {
            threshold_double(input, threshold, down, output, std::is_integral<T>());
        }

        template<class T>
        static void threshold_double( const Gray<T> &input , double threshold_d , bool down , Gray<U8> &output,
                                      std::true_type ) {
            auto threshold = static_cast<int64_t>(threshold_d);
            if( threshold < std::numeric_limits<T>::min() ) {
                output.reshape(input.width,input.height);
                ImageMiscOps::fill(output,static_cast<U8>(down ? 0 : 1));
            } else if( threshold >= std::numeric_limits<T>::max() ) {
                output.reshape(input.width,input.height);
                ImageMiscOps::fill(output,static_cast<U8>(down ? 1 : 0));
            } else {
                ThresholdOps::threshold(input,static_cast<T>(threshold),down,output);
            }
        }

        template<class T>
        static void threshold_double( const Gray<T> &input , double threshold , bool down , Gray<U8> &output,
                                      std::false_type ) {
            ThresholdOps::threshold(input,static_cast<float>(threshold),down,output);
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
         * @param storage1 Storage for intermediate step. Not used for U8
         * @param storage2 Storage for intermediate step. Not used for U8
         */
        template<class T>
        static void localMean( const Gray<T>& input , Gray<U8>& output ,
                               const ConfigLength& width , float scale , bool down ,
                               Gray<T>& storage1 , Gray<T>&  storage2 ) {
            if constexpr (std::is_same_v<T, U8>) {
                localMean_U8(input, output, width, scale, down);
                return;
            }

            output.reshape(input.width, input.height);
            storage1.reshape(input.width, input.height);
            storage2.reshape(input.width, input.height);

            uint32_t radius = (uint32_t)width.computeI(min(input.width,input.height))/2;

            Gray<T>& mean = storage1;

            BlurImageOps::mean(input,mean,radius,storage2);
            threshold_local(input, mean, scale, down, output);
        }

        /// Thresholds each pixel with its own threshold, from the 'local' image, e.g. a local mean:
        ///
        ///     down:  output(x,y) = input(x,y) <= local(x,y)*scale
        ///     up:    output(x,y) = input(x,y)*scale > local(x,y)
        ///
        /// 'local' and 'output' have the input's shape
        template<class T>
        static void threshold_local( const Gray<T>& input, const Gray<T>& local, float scale, bool down,
                                     Gray<U8>& output ) {
            for (uint32_t y = 0; y < input.height; y++) {
                const T* local_row = &local.data[local.offset + y*local.stride];
                threshold_local_row(&input.data[input.offset + y*input.stride],
                                    &output.data[output.offset + y*output.stride], input.width, scale, down,
                                    [=]( uint32_t x ) { return local_row[x]; });
            }
        }

        /// The output rows y0 to y1 of localMean() for U8, with the local region's radius. 'output' has the input's
        /// shape, and the radius and image size aren't 0. The sums start at row y0, so any range gives the same
        /// results
        static void localMean_U8_rows( const Gray<U8>& input, Gray<U8>& output, uint32_t radius, float scale,
                                       bool down, uint32_t y0, uint32_t y1 );

    private:
        /// threshold_local() of a row, with the threshold of pixel x from local(x)
        template<class T, class Local>
        static void threshold_local_row( const T* in, U8* out, uint32_t width, float scale, bool down, Local local ) {
            if (down) {
                for (uint32_t x = 0; x < width; x++)
                    out[x] = static_cast<U8>(in[x] <= local(x)*scale);
            } else {
                for (uint32_t x = 0; x < width; x++)
                    out[x] = static_cast<U8>(in[x]*scale > local(x));
            }
        }

        /// localMean() for U8, with the same results, in one pass over the image and without its storage:
        ///
        ///     for each row y:
        ///         horizontal means of the rows that enter the window next, 4 rows at a time, from prefix sums
        ///         sums[x] += the horizontal mean of the row entering the window - that of the row leaving it
        ///         out[x] = threshold of in[x] with sums[x]/(rows in the window)
        ///
        /// Only the horizontal means of the rows in the window, the row leaving it and those computed ahead are kept.
        /// See localMean_U8_rows()
        static void localMean_U8( const Gray<U8>& input, Gray<U8>& output, const ConfigLength& width, float scale,
                                  bool down );
    };


    /**
     * Interface for convert an input image into a binary image
     * @tparam T Type of input image.
     */
    template<class T>
    class InputToBinary {
    public:
        typedef typename T::pixel_type pixel_type;

        virtual ~InputToBinary() = default;

        virtual void process(const T& input , Gray<U8>& output ) = 0;
    };

    template<class T>
    class GlobalFixedBinaryFilter : public InputToBinary<Gray<T>>
    {
    public:
        // Converted to the image's type when applied
        double threshold;
        bool down;

        GlobalFixedBinaryFilter( double threshold, bool down ) :
                threshold(threshold), down(down)
        { }

        void process(const Gray<T>& input , Gray<U8>& output ) override {
            process_with<ThresholdOps>(input, output);
        }

        /// process() with the functions of Ops, ThresholdOps or ThresholdOps_MT
        template<class Ops>
        void process_with( const Gray<T>& input, Gray<U8>& output ) {
            Ops::threshold_double(input,threshold,down,output);
        }
    };

    template<class T>
    class GlobalOtsuBinaryFilter : public InputToBinary<Gray<T>>
    {
    public:
        // Bounds of the pixel values. See ThresholdOps::computeOtsu()
        double min_value;
        double max_value;
        bool down;
        // Scales the threshold
        double scale;

        GlobalOtsuBinaryFilter( double min_value , double max_value, bool down, double scale = 1.0 ) :
                min_value(min_value), max_value(max_value) , down(down), scale(scale)
        {  }

        void process(const Gray<T>& input , Gray<U8>& output ) override {
            process_with<ThresholdOps>(input, output);
        }

        /// process() with the functions of Ops, ThresholdOps or ThresholdOps_MT
        template<class Ops>
        void process_with( const Gray<T>& input, Gray<U8>& output ) {
            // BoofCV's GlobalBinaryFilter.process()
            double adjusted_scale = down ? scale : (scale <= 0.0 ? 0.0 : 1.0/scale);
            // computeOtsu() returns a histogram index
            double otsu = Ops::computeOtsu(input,min_value,max_value,false) + min_value;
            double threshold = otsu*adjusted_scale;
            Ops::threshold_double(input,threshold,down,output);
        }
    };

    template<class T>
    class LocalMeanBinaryFilter : public InputToBinary<Gray<T>>
    {
    public:
        ConfigLength regionWidth;
        float scale;
        bool down;
        Gray<T> storage1;
        Gray<T> storage2;

        LocalMeanBinaryFilter( const ConfigLength& regionWidth, float scale, bool down ) :
                scale(scale), down(down)
        {
            this->regionWidth = regionWidth;
        }

        void process(const Gray<T>& input , Gray<U8>& output ) override {
            process_with<ThresholdOps>(input, output);
        }

        /// process() with the functions of Ops, ThresholdOps or ThresholdOps_MT
        template<class Ops>
        void process_with( const Gray<T>& input, Gray<U8>& output ) {
            Ops::localMean(input,output,regionWidth,scale,down,storage1,storage2);
        }
    };
}

#endif