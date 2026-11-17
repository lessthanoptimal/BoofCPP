#ifndef BOOFCPP_IMAGE_STATISTICS_H
#define BOOFCPP_IMAGE_STATISTICS_H

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "boofcv/image_types.h"
#include "boofcv/base_types.h"

namespace boofcv {
    class ImageStatistics {
    public:
        template<class T>
        static T min( const Gray<T>& input) {
            T min_value = std::numeric_limits<T>::max();

            const T* ptr = input.data;
            const T* end = &ptr[input.total_pixels()];
            while( ptr != end ) {
                const T& v = *ptr++;
                if( v < min_value ) {
                    min_value = v;
                }
            }

            return min_value;
        }

        template<class T>
        static T max( const Gray<T>& input) {
            T max_value = std::numeric_limits<T>::min();

            const T* ptr = input.data;
            const T* end = &ptr[input.total_pixels()];
            while( ptr != end ) {
                const T& v = *ptr++;
                if( v > max_value ) {
                    max_value = v;
                }
            }

            return max_value;
        }

        template<class T>
        static typename TypeInfo<T>::sum_type sum( const Gray<T>& input) {
            typename TypeInfo<T>::sum_type total = 0;
            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t index = input.offset + y*input.stride;
                uint32_t end = index + input.width;
                while( index < end ) {
                    total += input.data[index++];
                }
            }
            return total;
        }

        template<class T>
        static T mean( const Gray<T>& input) {
            typename TypeInfo<T>::sum_type a = sum(input);
            return static_cast<T>(a / (input.width * input.height));
        }

        /// Histogram of pixel values. Bin i counts pixels equal to minValue + i. Throws std::out_of_range if a pixel
        /// doesn't have a bin, same as BoofCV. To avoid a branch per pixel, bins are clamped to one bin past the
        /// histogram, which is checked at the end. For unsigned integers
        template<class T, std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
        static void histogram( const Gray<T>& input , T minValue , GrowArray<uint32_t>& histogram ) {
            // Wide enough for any pixel value. Pixels below minValue wrap around to values past the histogram
            using W = std::common_type_t<uint32_t, T>;
            const uint32_t size = histogram.size;
            std::vector<uint32_t> guarded(size + 1, 0);

            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t index = input.offset + y*input.stride;
                uint32_t end = index + input.width;

                for( ; index < end; index++ ) {
                    W offset = static_cast<W>(input.data[index]) - static_cast<W>(minValue);
                    guarded[static_cast<uint32_t>(std::min(offset, static_cast<W>(size)))]++;
                }
            }

            if( guarded[size] != 0 )
                throw std::out_of_range("Pixel value outside of the histogram");
            for( uint32_t i = 0; i < size; i++ ) {
                histogram[i] = guarded[i];
            }
        }

        /// Histogram of pixel values. A pixel's bin is (int)(value - minValue), truncated toward zero, same as BoofCV.
        /// So values less than 1 outside the histogram are counted in its first or last bin, and NaN in the first.
        /// Throws std::out_of_range for other values. To avoid a branch per pixel, bins are clamped to [-1, size] and
        /// counted with a guard bin on each side. Only if a guard bin was used are pixels checked one by one.
        /// For F32 and F64
        template<class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
        static void histogram( const Gray<T>& input , T minValue , GrowArray<uint32_t>& histogram ) {
            const uint32_t size = histogram.size;
            const auto upper = static_cast<T>(size);
            std::vector<uint32_t> guarded(size + 2, 0);
            // guarded[0] counts NaN and values <= -1, guarded[size + 1] values >= size
            uint32_t* bins = guarded.data() + 1;

            for( uint32_t y = 0; y < input.height; y++ ) {
                uint32_t index = input.offset + y*input.stride;
                uint32_t end = index + input.width;

                for( ; index < end; index++ ) {
                    T offset = input.data[index] - minValue;
                    // max(-1, NaN) is -1. The operand order matches SSE's max and min instructions
                    bins[static_cast<int32_t>(std::min(upper, std::max(static_cast<T>(-1), offset)))]++;
                }
            }

            if( guarded[size + 1] != 0 )
                throw std::out_of_range("Pixel value outside of the histogram");
            uint32_t nan = 0;
            if( guarded[0] != 0 ) {
                for( uint32_t y = 0; y < input.height; y++ ) {
                    uint32_t index = input.offset + y*input.stride;
                    uint32_t end = index + input.width;

                    for( ; index < end; index++ ) {
                        T offset = input.data[index] - minValue;
                        if( offset != offset )
                            nan++;
                        else if( offset <= static_cast<T>(-1) )
                            throw std::out_of_range("Pixel value outside of the histogram");
                    }
                }
            }

            for( uint32_t i = 0; i < size; i++ ) {
                histogram[i] = bins[i];
            }
            histogram[0] += nan;
        }
    };
}

#endif