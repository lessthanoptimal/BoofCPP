#ifndef BOOFCPP_FAST_STEREO_DISPARITY_H
#define BOOFCPP_FAST_STEREO_DISPARITY_H

#include "boofcv/fast/disparity_block_match.h"
#include "boofcv/stereo_disparity.h"

#include <cstdint>

namespace boofcv::fast {
    // Strict's block matching pipelines
    using boofcv::StereoDisparityBlockMatch;
    using boofcv::StereoDisparityBlockMatchRowFormat;
    using boofcv::StereoDisparityBlockMatchCensus;
    using boofcv::StereoDisparityBlockMatchPreFilter;
    using boofcv::FactoryCensusTransform;
    using boofcv::CensusTransform;

    /// Fast's parts of block matching: its selectors, and 16-bit scores where they hold every region score. Results are
    /// the same as strict's
    struct BlockMatchParts {
        template<class S, class D>
        using SelectError = SelectErrorWithChecks<S, D>;

        template<class D>
        using SelectCorrelation = SelectCorrelationWithChecks_F32<D>;

        using SmallScore = uint16_t;
    };

    using FactoryStereoDisparity = boofcv::FactoryBlockMatch<BlockMatchParts>;
}

#endif
