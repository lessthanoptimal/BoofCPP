#ifndef BOOFCPP_STEREO_DISPARITY_MT_H
#define BOOFCPP_STEREO_DISPARITY_MT_H

#include "boofcv/census_transform_mt.h"
#include "boofcv/disparity_block_match_mt.h"
#include "boofcv/image_normalization_mt.h"
#include "boofcv/pixel_math_mt.h"
#include "boofcv/stereo_disparity.h"

namespace boofcv {

    /// Census transforms that use CensusTransform_MT
    using FactoryCensusTransform_MT = FactoryCensus<CensusTransform_MT>;

    /// BlockMatchParts with the concurrent parts, which give the same results for any number of threads
    struct BlockMatchParts_MT : BlockMatchParts {
        template<class E, class S>
        using RowFormat = DisparityBlockMatchRowFormat_MT<E, S>;

        using RowScoreZssd = BlockRowScoreZssd_MT;
        using RowScoreNcc = BlockRowScoreNcc_MT;
        using Census = CensusTransform_MT;
        using Normalization = ImageNormalization_MT;

        template<class I, class O>
        static void convert( const Gray<I>& input, Gray<O>& output ) { ConvertImage_MT::convert(input, output); }
    };

    /// Strict's concurrent block matching. Identical to FactoryStereoDisparity's for any number of threads
    using FactoryStereoDisparity_MT = FactoryBlockMatch<BlockMatchParts_MT>;
}

#endif
