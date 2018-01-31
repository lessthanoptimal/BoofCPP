#ifndef BOOFCPP_IMAGE_STATISTICS_H
#define BOOFCPP_IMAGE_STATISTICS_H

#include "images_types.h"
#include "base_types.h"

namespace boofcv {
    class ImageStatistics {
    public:
        template<class T>
        static T min( const Gray<T>& input) {
            T min_value = std::numeric_limits<T>::max();

            const T* ptr = input.data;
            const T* end = &ptr[input.numberOfPixels()];
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
            const T* end = &ptr[input.numberOfPixels()];
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

        template<class T>
        static void histogram( const Gray<T>& input , T minValue , GrowArray<uint32_t>& histogram ) {
            histogram.fill(0);

            for( int y = 0; y < input.height; y++ ) {
                uint32_t index = input.offset + y*input.stride;
                uint32_t end = index + input.width;

                for( ; index < end; index++ ) {
                    histogram[input.data[index] - minValue ]++;
                }
            }
        }
    };
}

#endif