#ifndef BOOFCPP_DISPARITY_BLOCK_MATCH_MT_H
#define BOOFCPP_DISPARITY_BLOCK_MATCH_MT_H

#include "boofcv/concurrency.h"
#include "boofcv/disparity_block_match.h"
#include "boofcv/image_blur_mt.h"
#include "boofcv/pixel_math_mt.h"

#include <memory>
#include <type_traits>

namespace boofcv {

    /// DisparityBlockMatchRowFormat with the output split into blocks of rows that threads take until none are left:
    ///
    ///     for each block of output rows, concurrently:
    ///         DisparityBlockMatchRowFormat's code on the block's rows, with its own workspace and selector copy
    ///
    /// Each block scores the region_height rows around its first row and sums them from scratch. Integer sums are
    /// exact, so a block can start on any row. With floating point scores a block sums from scratch at the multiple
    /// of catastrophic_reset at or before its first row, where the single threaded code also does, and runs the sum
    /// to its first row without selecting. BoofCV's DisparityScoreBM_F32 with USE_CONCURRENT starts its blocks on
    /// those multiples instead. The results are identical to DisparityBlockMatchRowFormat and BoofCV for any number
    /// of threads. With BoofConcurrency::max_threads() == 1 it's DisparityBlockMatchRowFormat::process()
    template<class E, class S>
    class DisparityBlockMatchRowFormat_MT : public DisparityBlockMatchRowFormat<E, S> {
        using Base = DisparityBlockMatchRowFormat<E, S>;
    public:
        using Base::Base;

        void process( const Gray<E>& left, const Gray<E>& right ) override {
            if (BoofConcurrency::max_threads() == 1) {
                Base::process(left, right);
                return;
            }
            this->prepare(left, right);

            // with floating point scores a block's sum starts at the multiple of reset at or before its first row
            const uint32_t reset = std::is_floating_point<S>::value ? (uint32_t)this->catastrophic_reset : 1;
            // Operations per score. A row: the error and box sum, the running sum's subtract and add, and the
            // selector. A block's extra work: scoring the 2*radius_y rows above its first row, and zeroing and summing
            // region_height rows from scratch
            const uint64_t scores = (uint64_t)this->width*this->get_disparity_range();
            const uint64_t row_cost = 7*scores;
            const uint64_t block_cost = (3*2*(uint64_t)this->radius_y + 2*this->region_height)*scores;
            // Every row costs about the same, so one block per thread is fastest. More blocks would each add the
            // extra rows
            BoofConcurrency::loop_blocks(0, this->height)
                    .schedule(Schedule::Fixed().multiplier(1))
                    .element_cost(row_cost)
                    .block_cost(block_cost)
                    .single([&] { this->process_all_rows(); })
                    .run([&]( uint32_t y_first, uint32_t y1 ) {
                        typename Base::Workspace workspace;
                        workspace.resize(*this);
                        std::unique_ptr<DisparitySelect<S>> select = this->select->concurrent_copy();
                        this->process_rows(workspace, *select, y_first - y_first%reset, y_first, y1);
                    });
        }
    };

    /// BlockRowScoreZssd with its local means from BlurImageOps_MT, which gives the same means for any number of
    /// threads
    class BlockRowScoreZssd_MT : public BlockRowScoreZssd {
    public:
        using BlockRowScoreZssd::BlockRowScoreZssd;

        void set_input( const Gray<F32>& left, const Gray<F32>& right ) override {
            BlurImageOps_MT::mean(left, mean_left, radius_x, radius_y, *border, storage);
            BlurImageOps_MT::mean(right, mean_right, radius_x, radius_y, *border, storage);
        }
    };

    /// BlockRowScoreNcc with its local statistics from BlurImageOps_MT and PixelMath_MT, which give the same
    /// statistics for any number of threads
    class BlockRowScoreNcc_MT : public BlockRowScoreNcc {
    public:
        using BlockRowScoreNcc::BlockRowScoreNcc;

        void set_input( const Gray<F32>& left, const Gray<F32>& right ) override {
            PixelMath_MT::pow2(left, tmp);
            BlurImageOps_MT::mean(tmp, stdev_left, radius_x, radius_y, *border, storage);
            PixelMath_MT::pow2(right, tmp);
            BlurImageOps_MT::mean(tmp, stdev_right, radius_x, radius_y, *border, storage);
            BlurImageOps_MT::mean(left, mean_left, radius_x, radius_y, *border, storage);
            BlurImageOps_MT::mean(right, mean_right, radius_x, radius_y, *border, storage);
            PixelMath_MT::stdev(mean_left, stdev_left, stdev_left);
            PixelMath_MT::stdev(mean_right, stdev_right, stdev_right);
        }
    };
}

#endif
