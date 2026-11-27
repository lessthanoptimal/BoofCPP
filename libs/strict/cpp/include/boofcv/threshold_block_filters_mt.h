#ifndef BOOFCPP_THRESHOLD_BLOCK_FILTERS_MT_H
#define BOOFCPP_THRESHOLD_BLOCK_FILTERS_MT_H

#include "boofcv/concurrency.h"
#include "boofcv/threshold_block_filters.h"

namespace boofcv {

    /// A block threshold filter, ThresholdBlockMean, ThresholdBlockMinMax or ThresholdBlockOtsu, with its rows of
    /// blocks split into bands, which threads take until none are left:
    ///
    ///     for each band of rows of blocks, concurrently:
    ///         a copy of the filter thresholds the band with ThresholdBlockCommon::process_blocks()
    ///
    /// With local blocks a band also computes the statistics of the row of blocks above and below it. A block's
    /// statistics don't depend on the band, so the results are identical for any number of threads. With
    /// BoofConcurrency::max_threads() == 1 it's the filter
    template<class Filter>
    class ThresholdBlock_MT : public Filter {
    public:
        using Filter::Filter;
        using E = typename Filter::pixel_type;

        /// A copy of 'filter', with its settings
        explicit ThresholdBlock_MT( const Filter& filter ) : Filter(filter) {}

        void process( const Gray<E>& input, Gray<U8>& output ) override {
            if (BoofConcurrency::max_threads() == 1) {
                Filter::process(input, output);
                return;
            }
            this->select_blocks(input, output);
            // Pixels per row of blocks, and the extra two rows of blocks of statistics each band computes
            const uint64_t row_cost = (uint64_t)this->blockHeight*input.width;
            BoofConcurrency::loop_blocks(0, this->blocksY)
                    .element_cost(2*row_cost)
                    .block_cost(this->thresholdFromLocalBlocks ? 2*row_cost : 0)
                    .single([&] {
                        this->begin(input.width);
                        this->process_blocks(input, output, 0, this->blocksY);
                    })
                    .run([&]( uint32_t b0, uint32_t b1 ) {
                        ThresholdBlock_MT band(*this);
                        band.begin(input.width);
                        band.process_blocks(input, output, b0, b1);
                    });
        }
    };
}

#endif
