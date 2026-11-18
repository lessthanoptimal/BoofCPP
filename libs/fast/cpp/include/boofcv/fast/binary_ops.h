#ifndef BOOFCPP_FAST_BINARY_OPS_H
#define BOOFCPP_FAST_BINARY_OPS_H

#include <cstdint>
#include <type_traits>

#include "boofcv/binary_ops.h"

namespace boofcv::fast {
    using boofcv::BinaryImageOps;
    using boofcv::ComputeOtsu;
    using boofcv::GlobalFixedBinaryFilter;
    using boofcv::InputToBinary;

    class ThresholdOps : public boofcv::ThresholdOps {
    public:
        /// Same as strict's ThresholdOps::computeOtsu(). U8 images fill several histograms at once
        template<class T>
        static uint32_t computeOtsu( const Gray<T>& input, double min_value, double max_value, bool otsu2 = false ) {
            if constexpr (std::is_same_v<T, U8>)
                return computeOtsu_U8(input, min_value, max_value, otsu2);
            else
                return boofcv::ThresholdOps::computeOtsu(input, min_value, max_value, otsu2);
        }

        /// Same as strict's ThresholdOps::localMean() for U8, in one pass over the image and without its storage
        static void localMean( const Gray<U8>& input, Gray<U8>& output, const ConfigLength& width, float scale,
                               bool down );

    private:
        static uint32_t computeOtsu_U8( const Gray<U8>& input, double min_value, double max_value, bool otsu2 );
    };

    /// Same results as strict's GlobalOtsuBinaryFilter. Faster for U8, see ThresholdOps::computeOtsu()
    template<class T>
    class GlobalOtsuBinaryFilter : public boofcv::GlobalOtsuBinaryFilter<T> {
    public:
        using boofcv::GlobalOtsuBinaryFilter<T>::GlobalOtsuBinaryFilter;

        void process( const Gray<T>& input, Gray<U8>& output ) override {
            if constexpr (std::is_same_v<T, U8>) {
                // Same math as strict's
                double adjusted_scale = this->down ? this->scale : (this->scale <= 0.0 ? 0.0 : 1.0/this->scale);
                double otsu = ThresholdOps::computeOtsu(input, this->min_value, this->max_value, false) +
                              this->min_value;
                ThresholdOps::threshold_double(input, otsu*adjusted_scale, this->down, output);
            } else {
                boofcv::GlobalOtsuBinaryFilter<T>::process(input, output);
            }
        }
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
