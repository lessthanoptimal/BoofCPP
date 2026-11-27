#ifndef BOOFCPP_MEAN_DIVISOR_H
#define BOOFCPP_MEAN_DIVISOR_H

#include <cstdint>
#include <limits>
#include <type_traits>

#include "boofcv/base_types.h"

namespace boofcv {
    /// Speeds up integer division by changing it into a multiplication and a shift. Multiply can be vectorized
    /// while an integer division can’t. This approach should apply to any modern CPU architecture.
    ///
    /// Turns the sum of 'length' pixels into their mean. Floats divide by the length and integers
    /// round to the nearest value, (total + length/2)/length.
    ///
    /// Dividing by a number that's only known at run time is slow. For unsigned integers the division is done with a
    /// multiplication and a shift that gives exactly the same result for every sum of 'length' pixels.
    template< class E>
    class MeanDivisor {
    public:
        typedef typename TypeInfo<E>::sum_type sum_type;

        explicit MeanDivisor( uint32_t length ) : divisor((sum_type)length) {
            if constexpr (std::is_unsigned<E>::value) {
                // Largest number that's divided. With n_max*length < 2^shift and multiplier = 2^shift/length + 1,
                // (n*multiplier) >> shift == n/length for every n <= n_max. n_max < 2^31 keeps n*multiplier < 2^64
                const uint64_t n_max = (uint64_t)std::numeric_limits<E>::max()*length + length/2;
                if (length == 0 || n_max >= (1ULL << 31))
                    return;
                const uint64_t limit = n_max*length;
                shift = 0;
                while (shift < 63 && (limit >> shift) != 0)
                    shift++;
                multiplier = ((uint64_t)1 << shift)/length + 1;
                use_multiply = true;
                // the product also fits in 32 bits, which can be computed for several pixels at once
                use_multiply32 = n_max*multiplier < (1ULL << 32) && shift < 32;
            }
        }

        E operator()( sum_type total ) const {
            if constexpr (std::is_floating_point<E>::value) {
                return static_cast<E>(total/divisor);
            } else {
                if constexpr (std::is_unsigned<E>::value) {
                    if (use_multiply)
                        return multiply(total);
                }
                return static_cast<E>((total + divisor/2)/divisor);
            }
        }

        /// True if operator() uses the multiplication instead of a division
        bool uses_multiply() const { return use_multiply; }

        /// Same as operator() when uses_multiply() is true
        E multiply( sum_type total ) const {
            return static_cast<E>(((uint64_t)(total + divisor/2)*multiplier) >> shift);
        }

        /// True if multiply32() can be used
        bool uses_multiply32() const { return use_multiply32; }

        /// Same as operator() when uses_multiply32() is true. The product is computed with 32 bits
        E multiply32( sum_type total ) const {
            return static_cast<E>(((uint32_t)(total + divisor/2)*(uint32_t)multiplier) >> shift);
        }

    private:
        sum_type divisor;
        bool use_multiply = false;
        bool use_multiply32 = false;
        uint64_t multiplier = 0;
        uint32_t shift = 0;
    };
}

#endif
