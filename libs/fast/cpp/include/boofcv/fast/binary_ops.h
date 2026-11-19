#ifndef BOOFCPP_FAST_BINARY_OPS_H
#define BOOFCPP_FAST_BINARY_OPS_H

#include <cstdint>
#include <type_traits>

#include "boofcv/binary_ops.h"

namespace boofcv::fast {
    using boofcv::BinaryImageOps;
    using boofcv::ComputeOtsu;
    using boofcv::GlobalFixedBinaryFilter;
    using boofcv::GlobalOtsuBinaryFilter;
    using boofcv::InputToBinary;

    /// Same results as strict's ComputeOtsu, but only the histogram's non-empty bins are visited. An empty bin leaves
    /// the background and foreground weights and sums unchanged, so its between class variance equals the one before
    /// it and can't be a new maximum
    class ComputeOtsuNonEmpty {
    public:
        double threshold = 0;
        double variance = 0;
        bool down;

        /// See strict's ComputeOtsu
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

    class ThresholdOps : public boofcv::ThresholdOps {
    public:
        /// Same as strict's ThresholdOps::localMean() for U8, in one pass over the image and without its storage
        static void localMean( const Gray<U8>& input, Gray<U8>& output, const ConfigLength& width, float scale,
                               bool down );
    };

    /// Same results as strict's LocalMeanBinaryFilter. Faster for U8, see ThresholdOps::localMean()
    template<class T>
    class LocalMeanBinaryFilter : public boofcv::LocalMeanBinaryFilter<T> {
    public:
        using boofcv::LocalMeanBinaryFilter<T>::LocalMeanBinaryFilter;

        void process( const Gray<T>& input, Gray<U8>& output ) override {
            if constexpr (std::is_same_v<T, U8>)
                ThresholdOps::localMean(input, output, this->regionWidth, this->scale, this->down);
            else
                boofcv::LocalMeanBinaryFilter<T>::process(input, output);
        }
    };
}

#endif
