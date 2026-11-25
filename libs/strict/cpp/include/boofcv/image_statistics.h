#ifndef BOOFCPP_IMAGE_STATISTICS_H
#define BOOFCPP_IMAGE_STATISTICS_H

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "boofcv/image_types.h"
#include "boofcv/base_types.h"

namespace boofcv {
    class ImageStatistics {
    public:
        /// Returns the lowest value out of all the pixels in the image
        template<class T>
        static T min( const Gray<T>& input) {
            return select(input, [](T v, T output) { return v < output; });
        }

        /// Returns the highest value out of all the pixels in the image
        template<class T>
        static T max( const Gray<T>& input) {
            return select(input, [](T v, T output) { return v > output; });
        }

        /// Type sum() returns. int64_t for integers, T for floats
        template<class T>
        using total_type = std::conditional_t<std::is_integral_v<T>, int64_t, T>;

        /// Returns the sum of all the pixels in the image
        template<class T>
        static total_type<T> sum( const Gray<T>& input) {
            // Rows of integers smaller than 32 bits are summed in an int32_t
            using row_type = std::conditional_t<std::is_integral_v<T> && (sizeof(T) < 4), int32_t, total_type<T>>;
            total_type<T> total = 0;
            for( uint32_t y = 0; y < input.height; y++ ) {
                const T* row = input.data + input.offset + y*input.stride;
                row_type row_total = 0;
                for( uint32_t x = 0; x < input.width; x++ )
                    row_total += row[x];
                total += row_total;
            }
            return total;
        }

        /// Returns the mean pixel value. double for integer images, T for floats
        template<class T>
        static auto mean( const Gray<T>& input) {
            using mean_type = std::conditional_t<std::is_integral_v<T>, double, T>;
            return static_cast<mean_type>(sum(input))/static_cast<mean_type>(input.width*input.height);
        }

        /// Histogram of pixel values. Bin i counts pixels equal to minValue + i, where minValue can be outside the
        /// pixel type's range. Throws std::out_of_range if a pixel doesn't have a bin. For unsigned integers. U8 uses
        /// histogram_U8(). The others avoid a branch per pixel:
        ///
        ///     for each pixel:  guarded[min(value - minValue, size)]++, where values below minValue wrap past size
        ///     throw if guarded[size] isn't 0
        template<class T, std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
        static void histogram( const Gray<T>& input , int32_t minValue , GrowArray<uint32_t>& histogram ) {
            if constexpr (std::is_same_v<T, U8>) {
                histogram_U8(input, minValue, histogram);
                return;
            }
            // Unsigned 32-bit arithmetic gives value - minValue for every bin, and pixels below minValue wrap around
            // to values past the histogram
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

        /// Histogram of pixel values. A pixel's bin is (int)(value - minValue), truncated toward zero.
        /// So values less than 1 outside the histogram are counted in its first or last bin, and NaN in the first.
        /// Throws std::out_of_range for other values. For F32 and F64. Avoids a branch per pixel:
        ///
        ///     for each pixel:  guarded[clamp(value - minValue, -1, size)]++, where NaN goes to -1
        ///     throw if guarded[size] isn't 0
        ///     if guarded[-1] isn't 0:  for each pixel:  count NaN, throw for other values below the histogram
        ///     bin 0 += NaN count
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

    private:
        /// Pixel 'output' is replaced by pixel 'v' when replace(v, output) is true
        template<class T, class Replace>
        static T select( const Gray<T>& input, Replace replace ) {
            T output = input.data[input.offset];
            for( uint32_t y = 0; y < input.height; y++ ) {
                const T* row = input.data + input.offset + y*input.stride;
                for( uint32_t x = 0; x < input.width; x++ ) {
                    if( replace(row[x], output) )
                        output = row[x];
                }
            }
            return output;
        }

        /// histogram() for U8. Several histograms keep repeated values from waiting on each other:
        ///
        ///     for each row:  pixel x counted in copy x%4, the row's last pixels in copy 0
        ///     for each value:  count = sum of its copies; if it isn't 0, throw if it has no bin, else move it there
        static void histogram_U8( const Gray<U8>& input, int32_t minValue, GrowArray<uint32_t>& histogram ) {
            constexpr uint32_t COPIES = 4;
            std::vector<uint32_t> copies(256*COPIES, 0);
            uint32_t* h = copies.data();
            const uint32_t width = input.width;
            for( uint32_t y = 0; y < input.height; y++ ) {
                const U8* row = &input.data[input.offset + y*input.stride];
                uint32_t x = 0;
                for( ; x + COPIES <= width; x += COPIES )
                    for( uint32_t k = 0; k < COPIES; k++ )
                        h[256*k + row[x + k]]++;
                for( ; x < width; x++ )
                    h[row[x]]++;
            }

            histogram.fill(0);
            for( int32_t value = 0; value < 256; value++ ) {
                uint32_t count = 0;
                for( uint32_t k = 0; k < COPIES; k++ )
                    count += h[256*k + value];
                if( count == 0 )
                    continue;
                const int64_t bin = (int64_t)value - minValue;
                if( bin < 0 || bin >= histogram.size )
                    throw std::out_of_range("Pixel value outside of the histogram");
                histogram[(uint32_t)bin] = count;
            }
        }
    };
}

#endif