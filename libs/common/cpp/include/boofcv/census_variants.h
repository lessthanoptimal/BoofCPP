#ifndef BOOFCPP_CENSUS_VARIANTS_H
#define BOOFCPP_CENSUS_VARIANTS_H

#include <cstdint>
#include <stdexcept>

namespace boofcv {
    /// Regions sampled by the census transform. BoofCV's CensusVariants
    enum class CensusVariants {
        BLOCK_3_3,
        BLOCK_5_5,
        BLOCK_7_7,
        BLOCK_9_7,
        BLOCK_13_5,
        CIRCLE_9
    };

    /// Java enum names of the CensusVariants values, in order
    inline const char* const CENSUS_VARIANTS_NAMES[] = {
            "BLOCK_3_3", "BLOCK_5_5", "BLOCK_7_7", "BLOCK_9_7", "BLOCK_13_5", "CIRCLE_9"};

    /// Number of bits in the census descriptor. BoofCV's CensusVariants.getBits()
    inline uint32_t census_bits( CensusVariants variant ) {
        switch (variant) {
            case CensusVariants::BLOCK_3_3: return 3*3 - 1;
            case CensusVariants::BLOCK_5_5: return 5*5 - 1;
            case CensusVariants::BLOCK_7_7: return 7*7 - 1;
            case CensusVariants::BLOCK_9_7: return 9*7 - 1;
            case CensusVariants::BLOCK_13_5: return 13*5 - 1;
            case CensusVariants::CIRCLE_9: return 9*9 - 4*6 - 1;
        }
        throw std::invalid_argument("Unknown census variant");
    }
}

#endif
