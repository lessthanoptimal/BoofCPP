#ifndef BOOFCPP_DISPARITY_BLOCK_MATCH_H
#define BOOFCPP_DISPARITY_BLOCK_MATCH_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "boofcv/image_types.h"
#include "boofcv/image_border.h"
#include "boofcv/census_transform.h"
#include "boofcv/image_blur.h"
#include "boofcv/pixel_math.h"

// Stereo disparity using block matching. BoofCV's DisparityScoreBM_S32 / DisparityScoreBM_F32, BlockRowScore and the
// SelectError / SelectCorrelation classes.
//
// Disparity convention: x_right = x_left - disparity. Scores for one image row are stored as
// scores[width*(d - disparity_min) + x], where x is the column in the left image.

namespace boofcv {

    /// Copies image row 'y' plus 'radius' border pixels on each side into 'output', which has width + 2*radius
    /// elements. Rows outside the image are read entirely from the border. BoofCV's GrowBorderSB.growRow().
    template<class E>
    void grow_row( const ImageBorder<E>& border, int32_t y, uint32_t radius, E* output ) {
        grow_row(border, y, -(int32_t)radius, (int32_t)(border.image->width + radius), output);
    }

    /// Computes the score of every region along an image row for each disparity. BoofCV's BlockRowScore.
    ///
    /// @tparam E Image pixel type
    /// @tparam S Score type
    template<class E, class S>
    class BlockRowScore {
    public:
        virtual ~BlockRowScore() = default;

        /// @param left_row Left image row with the border added on each side. width + region_width - 1 elements
        /// @param right_row Right image row with the border added on each side
        /// @param width Image width
        /// @param scores Output. scores[width*(d - disparity_min) + x]
        /// @param disparity_max Largest disparity, inclusive
        /// @param element_score Work space. width + region_width - 1 elements
        virtual void score_row( const E* left_row, const E* right_row, uint32_t width, S* scores,
                                int32_t disparity_min, int32_t disparity_max, uint32_t region_width,
                                S* element_score ) const = 0;

        /// Largest error a single pixel can have. Negative if there is no maximum.
        virtual int32_t max_per_pixel_error() const = 0;

        /// Called with the images before their rows are scored. BoofCV's BlockRowScore.setInput()
        virtual void set_input( const Gray<E>& left, const Gray<E>& right ) {
            (void)left;
            (void)right;
        }

        /// If true then the region scores must be passed through normalize_region_scores()
        virtual bool require_normalize() const { return false; }

        /// Normalization applied to the region scores, the sum of the row scores, before the disparity is selected.
        /// Only scores whose matched column is inside the image are written. BoofCV's
        /// BlockRowScore.normalizeRegionScores().
        ///
        /// @param row Image row of the region centers
        /// @param scores Region scores. scores[width*(d - disparity_min) + x]
        /// @param disparity_max Largest disparity, inclusive
        /// @param scores_norm Output. Normalized scores, same layout as scores
        void normalize_region_scores( int32_t row, const S* scores, int32_t disparity_min, int32_t disparity_max,
                                      uint32_t width_, uint32_t region_width, uint32_t region_height,
                                      S* scores_norm ) const {
            const int32_t width = width_;
            const int32_t disp_min = std::max(disparity_min, 1 - width);
            const int32_t disp_max = std::min(disparity_max, width - 1);
            for (int32_t d = disp_min; d <= disp_max; d++) {
                const int32_t col_left = std::max(0, d);
                const int32_t col_right = col_left - d;
                const int32_t num_cols = width - col_left - col_right;
                const int32_t index = width*(d - disparity_min) + col_left;
                normalize_score(row, col_left, col_right, num_cols, region_width, region_height,
                                scores + index, scores_norm + index);
            }
        }

    protected:
        /// Normalizes the scores of 'num_cols' regions in a row for one disparity. BoofCV's
        /// BlockRowScore.normalizeScore()
        ///
        /// @param col_left Column of the first region in the left image
        /// @param col_right Column of the first region in the right image
        virtual void normalize_score( int32_t row, int32_t col_left, int32_t col_right, int32_t num_cols,
                                      uint32_t region_width, uint32_t region_height,
                                      const S* scores, S* scores_norm ) const {
            (void)row, (void)col_left, (void)col_right, (void)num_cols, (void)region_width, (void)region_height;
            (void)scores, (void)scores_norm;
        }
    };

    /// Score type for an image type: int32_t for integer images and F32 for floating point images
    template<class E>
    using BlockScoreType = typename std::conditional<std::is_floating_point<E>::value, F32, int32_t>::type;

    /// Implements score_row() given the per pixel error. 'Error' is a functor and is inlined, so there are no virtual
    /// calls inside the loops.
    template<class E, class S, class Error>
    class BlockRowScoreError : public BlockRowScore<E, S> {
    public:
        void score_row( const E* left_row, const E* right_row, uint32_t width_, S* scores,
                        int32_t disparity_min, int32_t disparity_max, uint32_t region_width,
                        S* element_score ) const override {
            const int32_t width = width_;
            const int32_t width_and_border = width + region_width - 1;

            // Only disparities whose matched column stays inside the image can produce a score
            const int32_t disp_min = std::max(disparity_min, 1 - width);
            const int32_t disp_max = std::min(disparity_max, width - 1);

            for (int32_t d = disp_min; d <= disp_max; d++) {
                // The block's first left and right columns differ by the disparity, and one of them is always 0
                const int32_t col_left = std::max(0, d);
                const int32_t col_right = col_left - d;
                const int32_t num_cols = width - col_left - col_right;

                // error for each pixel
                const int32_t length = width_and_border - col_left - col_right;
                const E* __restrict a = left_row + col_left;
                const E* __restrict b = right_row + col_right;
                S* __restrict element = element_score;
                for (int32_t i = 0; i < length; i++)
                    element[i] = Error::compute(a[i], b[i]);

                // box sum along the row
                S score = 0;
                for (uint32_t i = 0; i < region_width; i++)
                    score += element[i];

                S* __restrict out = scores + width*(d - disparity_min) + col_left;
                out[0] = score;
                for (int32_t col = 0; col < num_cols - 1; col++) {
                    score += element[col + region_width] - element[col];
                    out[col + 1] = score;
                }
            }
        }
    };

    /// Absolute difference. BoofCV's BlockRowScoreSad
    template<class E>
    struct ErrorSad {
        static BlockScoreType<E> compute( E a, E b ) {
            if constexpr (std::is_floating_point<E>::value)
                return std::abs(a - b);
            else
                return std::abs((int32_t)a - (int32_t)b);
        }
    };

    /// Sum of absolute differences. BoofCV's BlockRowScoreSad
    ///
    /// @tparam S Score type. A smaller integer than BoofCV's gives the same scores if the largest one fits, since
    /// the running sums are then exact modulo its size
    template<class E, class S = BlockScoreType<E>>
    class BlockRowScoreSad : public BlockRowScoreError<E, S, ErrorSad<E>> {
    public:
        int32_t max_per_pixel_error() const override {
            return std::is_same<E, U8>::value ? 255 : -1;
        }
    };

    /// Squared difference. BoofCV's BlockRowScoreSsd
    template<class E>
    struct ErrorSsd {
        static BlockScoreType<E> compute( E a, E b ) {
            if constexpr (std::is_floating_point<E>::value) {
                E difference = a - b;
                return difference*difference;
            } else {
                int32_t difference = (int32_t)a - (int32_t)b;
                return difference*difference;
            }
        }
    };

    /// Sum of squared differences. BoofCV's BlockRowScoreSsd
    template<class E>
    class BlockRowScoreSsd : public BlockRowScoreError<E, BlockScoreType<E>, ErrorSsd<E>> {
    public:
        int32_t max_per_pixel_error() const override {
            return std::is_same<E, U8>::value ? 255*255 : -1;
        }
    };

    /// Zero mean sum of squared differences, computed from the SSD and the local means:
    /// ZSSD = SSD - N*(mean_left - mean_right)^2. BoofCV's BlockRowScoreZssd.F32
    class BlockRowScoreZssd : public BlockRowScoreError<F32, F32, ErrorSsd<F32>> {
    public:
        /// @param border How the local mean reads pixels outside the image
        BlockRowScoreZssd( uint32_t radius_x, uint32_t radius_y, BorderType border ) :
                radius_x(radius_x), radius_y(radius_y), border(FactoryImageBorder::create_SB<F32>(border)) {}

        int32_t max_per_pixel_error() const override { return -1; }

        void set_input( const Gray<F32>& left, const Gray<F32>& right ) override {
            BlurImageOps::mean(left, mean_left, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean(right, mean_right, radius_x, radius_y, *border, storage, totals);
        }

        bool require_normalize() const override { return true; }

    protected:
        void normalize_score( int32_t row, int32_t col_left, int32_t col_right, int32_t num_cols,
                              uint32_t region_width, uint32_t region_height,
                              const F32* scores, F32* scores_norm ) const override {
            const F32 area = (F32)(region_width*region_height);
            const F32* mean_l = mean_left.data + row*mean_left.stride + col_left;
            const F32* mean_r = mean_right.data + row*mean_right.stride + col_right;
            for (int32_t i = 0; i < num_cols; i++) {
                F32 delta_mean = mean_l[i] - mean_r[i];
                scores_norm[i] = scores[i] - area*delta_mean*delta_mean;
            }
        }

    protected:
        uint32_t radius_x, radius_y;
        std::shared_ptr<ImageBorder<F32>> border;
        Gray<F32> mean_left{1, 1}, mean_right{1, 1}, storage{1, 1};
        std::vector<F32> totals;
    };

    /// Product of two pixels. BoofCV's BlockRowScoreNcc
    struct ErrorProduct {
        static F32 compute( F32 a, F32 b ) { return a*b; }
    };

    /// Normalized cross correlation, computed from the sum of products and the local mean and standard deviation:
    /// NCC = (sum(L*R)/N - mean_l*mean_r)/(eps + sigma_l*sigma_r). Larger is better. BoofCV's
    /// BlockRowScoreNcc.F32
    class BlockRowScoreNcc : public BlockRowScoreError<F32, F32, ErrorProduct> {
    public:
        /// @param border How the local statistics read pixels outside the image
        /// @param eps Avoids dividing by zero
        BlockRowScoreNcc( uint32_t radius_x, uint32_t radius_y, BorderType border, F32 eps ) :
                radius_x(radius_x), radius_y(radius_y), eps(eps), border(FactoryImageBorder::create_SB<F32>(border)) {}

        int32_t max_per_pixel_error() const override { return -1; }

        /// Computes the local mean and standard deviation of each image
        void set_input( const Gray<F32>& left, const Gray<F32>& right ) override {
            // the mean of the squared pixels, then the mean, then the standard deviation in place of the squares
            PixelMath::pow2(left, tmp);
            BlurImageOps::mean(tmp, stdev_left, radius_x, radius_y, *border, storage, totals);
            PixelMath::pow2(right, tmp);
            BlurImageOps::mean(tmp, stdev_right, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean(left, mean_left, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean(right, mean_right, radius_x, radius_y, *border, storage, totals);
            PixelMath::stdev(mean_left, stdev_left, stdev_left);
            PixelMath::stdev(mean_right, stdev_right, stdev_right);
        }

        bool require_normalize() const override { return true; }

    protected:
        void normalize_score( int32_t row, int32_t col_left, int32_t col_right, int32_t num_cols,
                              uint32_t region_width, uint32_t region_height,
                              const F32* scores, F32* scores_norm ) const override {
            const F32 area = (F32)(region_width*region_height);
            const F32* mean_l = mean_left.data + row*mean_left.stride + col_left;
            const F32* mean_r = mean_right.data + row*mean_right.stride + col_right;
            const F32* sigma_l = stdev_left.data + row*stdev_left.stride + col_left;
            const F32* sigma_r = stdev_right.data + row*stdev_right.stride + col_right;
            for (int32_t i = 0; i < num_cols; i++) {
                F32 correlation = scores[i]/area;
                scores_norm[i] = (correlation - mean_l[i]*mean_r[i])/(eps + sigma_l[i]*sigma_r[i]);
            }
        }

    protected:
        uint32_t radius_x, radius_y;
        F32 eps;
        std::shared_ptr<ImageBorder<F32>> border;
        Gray<F32> mean_left{1, 1}, mean_right{1, 1}, stdev_left{1, 1}, stdev_right{1, 1}, tmp{1, 1}, storage{1, 1};
        std::vector<F32> totals;
    };

    /// Hamming distance between two census descriptors
    template<class E>
    struct ErrorCensus {
        static int32_t compute( E a, E b ) {
            if constexpr (sizeof(E) == 8)
                return hamming((uint64_t)(a ^ b));
            else
                return hamming((uint32_t)(a ^ b));
        }
    };

    /// Hamming distance between census transformed images. BoofCV's BlockRowScoreCensus.
    ///
    /// @tparam E Census image type. U8, S32 or S64
    /// @tparam S Score type, see BlockRowScoreSad
    template<class E, class S = int32_t>
    class BlockRowScoreCensus : public BlockRowScoreError<E, S, ErrorCensus<E>> {
    public:
        static_assert(std::is_same<E, U8>::value || std::is_same<E, S32>::value || std::is_same<E, S64>::value,
                      "Census image must be U8, S32 or S64");

        /// @param bits Number of bits in the census descriptor
        explicit BlockRowScoreCensus( int32_t bits ) : bits(bits) {}

        int32_t max_per_pixel_error() const override { return bits; }

    private:
        int32_t bits;
    };

    /// Selects the best disparity for each pixel in a row from the scores. BoofCV's DisparitySelect.
    template<class S>
    class DisparitySelect {
    public:
        virtual ~DisparitySelect() = default;

        /// @param disparity_max Largest disparity, inclusive
        virtual void configure( int32_t disparity_min, int32_t disparity_max, uint32_t width ) = 0;

        /// Selects the disparity for every pixel in output row 'row'
        virtual void process( uint32_t row, const S* scores ) = 0;

        /// A copy, with its settings and output, that another thread can call process() on for different rows.
        /// BoofCV's DisparitySelect.concurrentCopy()
        virtual std::unique_ptr<DisparitySelect<S>> concurrent_copy() const = 0;
    };

    /// Where a selector writes the disparity and the score
    ///
    /// @tparam D Disparity image type, U8 or F32
    template<class D>
    class DisparityOutput {
    public:
        virtual ~DisparityOutput() = default;

        /// Where the disparity and optionally the score are written. Must have the input image's shape.
        void set_output( Gray<D>& disparity, Gray<F32>* score ) {
            this->disparity = &disparity;
            this->score = score;
        }

    protected:
        Gray<D>* disparity = nullptr;
        Gray<F32>* score = nullptr;
    };

    /// Best disparity of each right image column c, over the left columns c + d + disparity_min inside the image,
    /// searched in increasing order: the first lowest score, or the first largest if LARGEST. Loops
    /// over every column for one disparity at a time, contiguous in the score layout scores[width*d + x]
    ///
    /// @param score Work space. Best score of each column
    /// @param best Output. Disparity of each column, relative to disparity_min
    template<bool LARGEST, class S, class I>
    void select_right_to_left( const S* scores, int32_t width, int32_t disparity_min, int32_t disparity_range,
                               S* __restrict score, I* __restrict best ) {
        for (int32_t c = 0; c < width; c++) {
            const int32_t first = std::max(0, -c - disparity_min);
            if (first < disparity_range && c + first + disparity_min < width) {
                score[c] = scores[width*first + c + first + disparity_min];
                best[c] = (I)first;
            }
        }
        for (int32_t d = 0; d < disparity_range; d++) {
            const int32_t dd = d + disparity_min;
            const int32_t c0 = std::max(0, -dd);
            const int32_t c1 = std::min(width, width - dd);
            const S* __restrict s = scores + width*d + dd;
            for (int32_t c = c0; c < c1; c++) {
                // the index is blended with a mask, a form compilers vectorize for float scores too
                const S v = s[c];
                const S current = score[c];
                const bool better = LARGEST ? v > current : v < current;
                const I mask = (I)((I)0 - (I)better);
                score[c] = better ? v : current;
                best[c] = (I)((best[c] & (I)~mask) | ((I)d & mask));
            }
        }
    }

    /// Winner takes all with optional checks for maximum error, right to left consistency, and texture. Same as
    /// BoofCV's SelectErrorWithChecks_S32 and SelectErrorWithChecks_F32. If the disparity type 'D' is F32 then the
    /// disparity is refined to subpixel precision, BoofCV's SelectErrorSubpixel.
    ///
    /// Each loop goes over every column for one disparity at a time, which is contiguous in the score layout
    /// scores[width*(d - disparity_min) + x], so the compiler vectorizes it:
    ///
    ///     for each disparity d:  for each column:  best = d if its score is lower, which keeps the first minimum
    ///     with right to left:  the same for each right image column, see select_right_to_left()
    ///     for each column:  invalid if the score is above max_error or right to left disagrees
    ///     with texture:  for each disparity d:  for each column:  second = lowest score not next to or at best
    ///     for each column:  invalid if second isn't enough above the score, then write the disparity
    ///
    /// @tparam S Score type: int32_t, uint16_t or F32. With uint16_t the scores and the disparity range must be below
    /// 65535. The results are the same as with int32_t
    /// @tparam D Disparity image type, U8 or F32
    template<class S, class D>
    class SelectErrorWithChecks : public DisparitySelect<S>, public DisparityOutput<D> {
    public:
        /// Integer texture thresholds are scaled by this
        static constexpr int32_t DISCRETIZER = 10000;

        /// @param max_error Maximum allowed region error. <= 0 means no limit.
        /// @param right_to_left_tolerance Tolerance for right to left validation. < 0 disables it.
        /// @param texture Texture threshold. <= 0 disables it.
        /// @param squared_error true if the error is squared. Only used for subpixel.
        SelectErrorWithChecks( double max_error, int32_t right_to_left_tolerance, double texture,
                               bool squared_error = false ) :
                right_to_left_tolerance(right_to_left_tolerance), squared_error(squared_error) {
            const bool no_limit = max_error <= 0 || max_error == std::numeric_limits<double>::max();
            if constexpr (std::is_floating_point<S>::value) {
                max_error_s = no_limit ? std::numeric_limits<F32>::max() : (F32)max_error;
                texture_threshold = (F32)texture;
            } else {
                max_error_s = no_limit ? std::numeric_limits<int32_t>::max() : (int32_t)max_error;
                texture_threshold = (int32_t)(DISCRETIZER*texture);
            }
        }

        void configure( int32_t disparity_min, int32_t disparity_max, uint32_t width ) override {
            this->disparity_min = disparity_min;
            this->disparity_max = disparity_max;
            disparity_range = disparity_max - disparity_min + 1;
            image_width = width;
            invalid_disparity = disparity_range;
            if (std::is_same<D, U8>::value && invalid_disparity > 254)
                throw std::invalid_argument("Max range exceeds maximum value in disparity image");
            if (std::is_same<I, uint16_t>::value && invalid_disparity >= 0xFFFF)
                throw std::invalid_argument("Disparity range is too large for 16-bit scores");
            best_score.resize(width);
            best.resize(width);
            r2l_score.resize(width);
            r2l_best.resize(width);
            second.resize(width);
        }

        std::unique_ptr<DisparitySelect<S>> concurrent_copy() const override {
            return std::make_unique<SelectErrorWithChecks<S, D>>(*this);
        }

        void process( uint32_t row, const S* scores ) override {
            Gray<D>* disparity = this->disparity;
            Gray<F32>* score = this->score;
            D* out = disparity->data + disparity->offset + row*disparity->stride;
            F32* out_score = score == nullptr ? nullptr : score->data + score->offset + row*score->stride;
            const int32_t width = image_width;
            // first and last columns it will consider
            const int32_t col0 = std::max(0, disparity_min);
            const int32_t col1 = width + std::min(0, disparity_max);

            select_best(scores, col0, col1);
            if (right_to_left_tolerance >= 0)
                select_right_to_left<false>(scores, width, disparity_min, disparity_range, r2l_score.data(),
                                            r2l_best.data());

            // max error and right to left checks. best then holds the selected disparity, valid or not
            I* __restrict b = best.data();
            for (int32_t col = col0; col < col1; col++) {
                if (best_score[col] > max_error_s) {
                    b[col] = (I)invalid_disparity;
                } else if (right_to_left_tolerance >= 0) {
                    const int32_t value = b[col];
                    if (std::abs((int32_t)r2l_best[col - value - disparity_min] - value) > right_to_left_tolerance)
                        b[col] = (I)invalid_disparity;
                }
            }
            if (texture_threshold > 0)
                select_second(scores, col0, col1);

            for (int32_t col = 0; col < col0; col++)
                out[col] = (D)invalid_disparity;
            for (int32_t col = col0; col < col1; col++) {
                // first valid disparity and the past-the-end disparity (both relative to disparity_min)
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                const int32_t local_range = std::min(col, disparity_max) - disparity_min + 1;
                int32_t value = b[col];
                const S score_best = best_score[col];
                if (texture_threshold > 0 && value != invalid_disparity && local_range - local_min >= 3) {
                    if constexpr (std::is_floating_point<S>::value) {
                        if (second[col] - score_best <= texture_threshold*score_best)
                            value = invalid_disparity;
                    } else {
                        // No candidate is the type's maximum, which is int32_t's in BoofCV
                        const int64_t second_best = second[col] == std::numeric_limits<S>::max() ?
                                                    std::numeric_limits<int32_t>::max() : (int64_t)second[col];
                        if ((int64_t)DISCRETIZER*(second_best - score_best) <= (int64_t)texture_threshold*score_best)
                            value = invalid_disparity;
                    }
                }
                set_disparity(out, col, value, local_min, local_range, scores);
                if (out_score != nullptr)
                    out_score[col] = (F32)score_best;
            }
            for (int32_t col = col1; col < width; col++)
                out[col] = (D)invalid_disparity;
        }

    private:
        /// Disparity index type, the same size as the score so the loops use one vector width
        using I = typename std::conditional<sizeof(S) == 2, uint16_t, int32_t>::type;
        /// Maximum error is compared in BoofCV's score type
        using Limit = typename std::conditional<std::is_floating_point<S>::value, F32, int32_t>::type;

        int32_t disparity_min = 0, disparity_max = 0, disparity_range = 0;
        int32_t image_width = 0;
        int32_t invalid_disparity = 0;

        Limit max_error_s;
        int32_t right_to_left_tolerance;
        Limit texture_threshold;
        bool squared_error;

        // best and second best score of each column, and the best of each right image column
        std::vector<S> best_score, r2l_score, second;
        std::vector<I> best, r2l_best;

        /// Lowest score of each column and its disparity. Disparity d (relative) is valid for the columns
        /// [max(0, d + disparity_min), min(width, width + d + disparity_min))
        void select_best( const S* scores, int32_t col0, int32_t col1 ) {
            const int32_t width = image_width;
            I* __restrict b = best.data();
            S* __restrict bs = best_score.data();
            for (int32_t col = col0; col < col1; col++) {
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                bs[col] = scores[width*local_min + col];
                b[col] = (I)local_min;
            }
            for (int32_t d = 0; d < disparity_range; d++) {
                const int32_t dd = d + disparity_min;
                const int32_t c0 = std::max(col0, std::max(0, dd));
                const int32_t c1 = std::min(col1, std::min(width, width + dd));
                const S* __restrict s = scores + width*d;
                for (int32_t c = c0; c < c1; c++) {
                    // the index is blended with a mask, a form compilers vectorize for float scores too
                    const S v = s[c];
                    const S current = bs[c];
                    const I mask = (I)((I)0 - (I)(v < current));
                    bs[c] = v < current ? v : current;
                    b[c] = (I)((b[c] & (I)~mask) | ((I)d & mask));
                }
            }
        }

        /// Lowest score of each column, excluding the selected disparity and its two neighbors. The type's maximum if
        /// there's none. Columns which are invalid aren't used afterwards
        void select_second( const S* scores, int32_t col0, int32_t col1 ) {
            const int32_t width = image_width;
            S* __restrict sec = second.data();
            const I* __restrict b = best.data();
            for (int32_t col = col0; col < col1; col++)
                sec[col] = std::numeric_limits<S>::max();
            for (int32_t d = 0; d < disparity_range; d++) {
                const int32_t dd = d + disparity_min;
                const int32_t c0 = std::max(col0, std::max(0, dd));
                const int32_t c1 = std::min(col1, std::min(width, width + dd));
                const S* __restrict s = scores + width*d;
                for (int32_t c = c0; c < c1; c++) {
                    // d < b - 1 or d > b + 1, written without a subtraction so it can't wrap
                    const S v = s[c];
                    const bool use = ((I)(d + 1) < b[c] || (I)d > (I)(b[c] + 1)) && v < sec[c];
                    sec[c] = use ? v : sec[c];
                }
            }
        }

        void set_disparity( D* out, int32_t col, int32_t value, int32_t local_min, int32_t local_range,
                            const S* scores ) const {
            if constexpr (std::is_same<D, F32>::value) {
                if (value <= local_min || value >= local_range - 1) {
                    out[col] = (F32)value;
                } else {
                    // Fit a parabola to the scores around the best one
                    const int32_t width = image_width;
                    F32 c0 = (F32)scores[width*(value - 1) + col];
                    F32 c1 = (F32)scores[width*value + col];
                    F32 c2 = (F32)scores[width*(value + 1) + col];
                    if (!squared_error) {
                        c0 *= c0;
                        c1 *= c1;
                        c2 *= c2;
                    }
                    F32 offset = (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
                    out[col] = (F32)value + offset;
                }
            } else {
                (void)local_min, (void)local_range, (void)scores;
                out[col] = (D)value;
            }
        }
    };

    /// Selects the disparity with the largest correlation, with optional checks for right to left consistency and
    /// texture. BoofCV's SelectCorrelationWithChecks_F32. If the disparity type 'D' is F32 then the disparity
    /// is refined to subpixel precision, BoofCV's SelectCorrelationSubpixel.F32_F32.
    ///
    /// Columns which can't be estimated have a score of NaN. If the texture check is done, the saved score is
    /// relative to the worst score.
    ///
    /// The loops go over every column for one disparity at a time, like SelectErrorWithChecks. The largest score wins,
    /// the first one if there are several
    ///
    /// @tparam D Disparity image type, U8 or F32
    template<class D>
    class SelectCorrelationWithChecks_F32 : public DisparitySelect<F32>, public DisparityOutput<D> {
    public:
        /// @param right_to_left_tolerance Tolerance for right to left validation. < 0 disables it.
        /// @param texture Texture threshold. <= 0 disables it.
        SelectCorrelationWithChecks_F32( int32_t right_to_left_tolerance, double texture ) :
                right_to_left_tolerance(right_to_left_tolerance), texture_threshold((F32)texture) {}

        void configure( int32_t disparity_min, int32_t disparity_max, uint32_t width ) override {
            this->disparity_min = disparity_min;
            this->disparity_max = disparity_max;
            disparity_range = disparity_max - disparity_min + 1;
            image_width = width;
            invalid_disparity = disparity_range;
            if (std::is_same<D, U8>::value && invalid_disparity > 254)
                throw std::invalid_argument("Max range exceeds maximum value in disparity image");
            best_score.resize(width);
            worst_score.resize(width);
            best.resize(width);
            r2l_score.resize(width);
            r2l_best.resize(width);
            second.resize(width);
        }

        std::unique_ptr<DisparitySelect<F32>> concurrent_copy() const override {
            return std::make_unique<SelectCorrelationWithChecks_F32<D>>(*this);
        }

        void process( uint32_t row, const F32* scores ) override {
            Gray<D>* disparity = this->disparity;
            Gray<F32>* score = this->score;
            D* out = disparity->data + disparity->offset + row*disparity->stride;
            F32* out_score = score == nullptr ? nullptr : score->data + score->offset + row*score->stride;
            const int32_t width = image_width;
            const F32 nan = std::numeric_limits<F32>::quiet_NaN();
            const int32_t col0 = std::max(0, disparity_min);
            const int32_t col1 = width + std::min(0, disparity_max);

            select_best(scores, col0, col1);
            int32_t* __restrict b = best.data();
            if (right_to_left_tolerance >= 0) {
                select_right_to_left<true>(scores, width, disparity_min, disparity_range, r2l_score.data(),
                                           r2l_best.data());
                for (int32_t col = col0; col < col1; col++) {
                    const int32_t value = b[col];
                    if (std::abs(r2l_best[col - value - disparity_min] - value) > right_to_left_tolerance)
                        b[col] = invalid_disparity;
                }
            }
            if (texture_threshold > 0)
                select_second(scores, col0, col1);

            for (int32_t col = 0; col < col0; col++)
                set_disparity(out, out_score, col, invalid_disparity, nan, 0, 0, scores);
            for (int32_t col = col0; col < col1; col++) {
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                const int32_t local_range = std::min(col, disparity_max) - disparity_min + 1;
                int32_t value = b[col];
                F32 score_best = best_score[col];
                if (texture_threshold > 0 && value != invalid_disparity && local_range - local_min >= 3) {
                    // relative to the worst score
                    const F32 score_worst = worst_score[col];
                    score_best -= score_worst;
                    const F32 second_best = second[col] - score_worst;
                    if (score_best - second_best <= texture_threshold*second_best)
                        value = invalid_disparity;
                }
                set_disparity(out, out_score, col, value, score_best, local_min, local_range, scores);
            }
            for (int32_t col = col1; col < width; col++)
                set_disparity(out, out_score, col, invalid_disparity, nan, 0, 0, scores);
        }

    private:
        int32_t disparity_min = 0, disparity_max = 0, disparity_range = 0;
        int32_t image_width = 0;
        int32_t invalid_disparity = 0;

        int32_t right_to_left_tolerance;
        F32 texture_threshold;

        std::vector<F32> best_score, worst_score, r2l_score, second;
        std::vector<int32_t> best, r2l_best;

        /// Largest and smallest score of each column, and the disparity of the largest. BoofCV updates the worst only
        /// for scores which aren't a new best, and those can't be smaller than the worst, so it's the minimum
        void select_best( const F32* scores, int32_t col0, int32_t col1 ) {
            const int32_t width = image_width;
            int32_t* __restrict b = best.data();
            F32* __restrict bs = best_score.data();
            F32* __restrict ws = worst_score.data();
            for (int32_t col = col0; col < col1; col++) {
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                bs[col] = ws[col] = scores[width*local_min + col];
                b[col] = local_min;
            }
            for (int32_t d = 0; d < disparity_range; d++) {
                const int32_t dd = d + disparity_min;
                const int32_t c0 = std::max(col0, std::max(0, dd));
                const int32_t c1 = std::min(col1, std::min(width, width + dd));
                const F32* __restrict s = scores + width*d;
                for (int32_t c = c0; c < c1; c++) {
                    const F32 v = s[c];
                    const F32 current = bs[c];
                    const int32_t mask = -(int32_t)(v > current);
                    bs[c] = v > current ? v : current;
                    b[c] = (b[c] & ~mask) | (d & mask);
                    ws[c] = v < ws[c] ? v : ws[c];
                }
            }
        }

        /// Largest score of each column excluding the selected disparity and its two neighbors, starting from the
        /// worst score
        void select_second( const F32* scores, int32_t col0, int32_t col1 ) {
            const int32_t width = image_width;
            F32* __restrict sec = second.data();
            const int32_t* __restrict b = best.data();
            for (int32_t col = col0; col < col1; col++)
                sec[col] = worst_score[col];
            for (int32_t d = 0; d < disparity_range; d++) {
                const int32_t dd = d + disparity_min;
                const int32_t c0 = std::max(col0, std::max(0, dd));
                const int32_t c1 = std::min(col1, std::min(width, width + dd));
                const F32* __restrict s = scores + width*d;
                for (int32_t c = c0; c < c1; c++) {
                    const F32 v = s[c];
                    const bool use = (d < b[c] - 1 || d > b[c] + 1) && v > sec[c];
                    sec[c] = use ? v : sec[c];
                }
            }
        }

        void set_disparity( D* out, F32* out_score, int32_t col, int32_t value, F32 score_value,
                            int32_t local_min, int32_t local_range, const F32* scores ) const {
            if constexpr (std::is_same<D, F32>::value) {
                if (value <= local_min || value >= local_range - 1) {
                    out[col] = (F32)value;
                } else {
                    // Fit a parabola to the scores around the best one
                    const int32_t width = image_width;
                    F32 c0 = scores[width*(value - 1) + col];
                    F32 c1 = scores[width*value + col];
                    F32 c2 = scores[width*(value + 1) + col];
                    F32 offset = (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
                    out[col] = (F32)value + offset;
                }
            } else {
                (void)local_min, (void)local_range, (void)scores;
                out[col] = (D)value;
            }
            if (out_score != nullptr)
                out_score[col] = score_value;
        }
    };

    template<class D>
    using SelectErrorWithChecks_S32 = SelectErrorWithChecks<int32_t, D>;

    template<class D>
    using SelectErrorWithChecks_F32 = SelectErrorWithChecks<F32, D>;
    /// Block matching of two images, which writes the disparity through its selector. Lets a caller hold block
    /// matching of any score type
    ///
    /// @tparam E Image pixel type
    template<class E>
    class DisparityBlockMatch {
    public:
        virtual ~DisparityBlockMatch() = default;

        virtual void process( const Gray<E>& left, const Gray<E>& right ) = 0;

        virtual int32_t get_disparity_min() const = 0;

        virtual uint32_t get_disparity_range() const = 0;

        virtual uint32_t get_radius_y() const = 0;
    };

    /// Block matching which computes the score one row at a time and reuses the previous row's scores with a
    /// running sum. BoofCV's DisparityScoreBM_S32 and DisparityScoreBM_F32. With floating point scores the running sum
    /// is recomputed from scratch every 'catastrophic_reset' rows.
    ///
    /// @tparam E Image pixel type
    /// @tparam S Score type
    template<class E, class S>
    class DisparityBlockMatchRowFormat : public DisparityBlockMatch<E> {
    public:
        const uint32_t radius_x, radius_y;
        const uint32_t region_width, region_height;

        DisparityBlockMatchRowFormat( uint32_t radius_x, uint32_t radius_y,
                                      std::shared_ptr<BlockRowScore<E, S>> row_score,
                                      std::shared_ptr<DisparitySelect<S>> select ) :
                radius_x(radius_x), radius_y(radius_y),
                region_width(radius_x*2 + 1), region_height(radius_y*2 + 1),
                select(std::move(select)), row_score(std::move(row_score)) {}

        /// @param disparity_range Number of disparity values. Max disparity is min + range - 1
        void configure( int32_t disparity_min, uint32_t disparity_range ) {
            if (disparity_range == 0)
                throw std::invalid_argument("Disparity range must be more than 0");
            this->disparity_min = disparity_min;
            this->disparity_max = disparity_min + (int32_t)disparity_range - 1;
            this->disparity_range = disparity_range;
        }

        int32_t get_disparity_min() const override { return disparity_min; }

        uint32_t get_disparity_range() const override { return disparity_range; }

        uint32_t get_radius_y() const override { return radius_y; }

        /// Rows between recomputing the floating point running sum from scratch. BoofCV's
        /// DisparityBlockMatch.catastrophicReset. Not used with integer scores, where it doesn't change the result
        void set_catastrophic_reset( int32_t rows ) {
            if (rows <= 0)
                throw std::invalid_argument("catastrophic_reset must be > 0");
            catastrophic_reset = rows;
        }

        /// The two borders are separate instances because each one references its own image
        void set_border( BorderType type ) {
            border_left = FactoryImageBorder::create_SB<E>(type);
            border_right = FactoryImageBorder::create_SB<E>(type);
        }

        /// Computes the score for every pixel and passes them to the selector, one row at a time.
        void process( const Gray<E>& left, const Gray<E>& right ) override {
            prepare(left, right);
            process_all_rows();
        }

    protected:
        /// Running sums of the scores and copies of the image rows, for one thread
        struct Workspace {
            // horizontal scores of the last region_height rows. Row y is in horizontal[(y + radius_y)%region_height].
            // vertical is their sum
            std::vector<std::vector<S>> horizontal;
            std::vector<S> vertical;
            // vertical after normalization, if the score requires it
            std::vector<S> vertical_norm;
            std::vector<S> element;
            std::vector<E> left_row, right_row;

            /// Sizes the arrays for the image given to prepare()
            void resize( const DisparityBlockMatchRowFormat& alg ) {
                const uint32_t block = alg.width*alg.disparity_range;
                horizontal.assign(alg.region_height, std::vector<S>(block, 0));
                vertical.assign(block, 0);
                if (alg.row_score->require_normalize())
                    vertical_norm.assign(block, 0);
                element.resize(alg.width + 2*alg.radius_x);
                left_row.resize(alg.width + 2*alg.radius_x);
                right_row.resize(alg.width + 2*alg.radius_x);
            }
        };

        std::shared_ptr<DisparitySelect<S>> select;
        int32_t catastrophic_reset = 25;
        uint32_t width = 0, height = 0;   // set by prepare()

        /// Sets up the borders, the row score and the selector for the images
        void prepare( const Gray<E>& left, const Gray<E>& right ) {
            checkSameShape(left, right);

            border_left->setImage(left);
            border_right->setImage(right);
            row_score->set_input(left, right);

            width = left.width;
            height = left.height;
            row_disparity_max = std::min((int32_t)width, disparity_max);
            select->configure(disparity_min, disparity_max, width);
        }

        /// Every row, after prepare()
        void process_all_rows() {
            workspace.resize(*this);
            process_rows(workspace, *select, 0, 0, height);
        }

        /// Output rows y_first <= y < y1, after prepare(). The vertical sum starts from scratch at y0 <= y_first and
        /// runs to y_first without selecting. Results are the same as one call for every row if y0 is 0 or, with
        /// floating point scores, a multiple of catastrophic_reset. The running sums of integer scores are exact, so
        /// any y0 gives the same results for them
        void process_rows( Workspace& ws, DisparitySelect<S>& out, uint32_t y0, uint32_t y_first, uint32_t y1 ) const {
            const uint32_t block = width*disparity_range;
            const int32_t row1 = (int32_t)y1 + (int32_t)radius_y;

            // the rows of the region centered on y0, then their sum
            for (uint32_t i = 0; i < region_height; i++) {
                const int32_t row = (int32_t)(y0 + i) - (int32_t)radius_y;
                score_image_row(ws, row, ws.horizontal[(y0 + i)%region_height].data());
            }
            sum_from_scratch(ws);
            if (y0 == y_first)
                select_row(ws, out, y0);

            // remaining rows: subtract the oldest row and add the new one
            for (int32_t row = (int32_t)(y0 + region_height) - (int32_t)radius_y; row < row1; row++) {
                S* h = ws.horizontal[(row + radius_y)%region_height].data();
                subtract(ws.vertical.data(), h, block);
                score_image_row(ws, row, h);
                if (std::is_floating_point<S>::value && (row - (int32_t)radius_y)%catastrophic_reset == 0) {
                    // Restarts the sum to limit the floating point error
                    sum_from_scratch(ws);
                } else {
                    add(ws.vertical.data(), h, block);
                }
                if (row - (int32_t)radius_y >= (int32_t)y_first)
                    select_row(ws, out, row - (int32_t)radius_y);
            }
        }

    private:
        int32_t disparity_min = 0;
        int32_t disparity_max = 0;
        uint32_t disparity_range = 0;

        std::shared_ptr<BlockRowScore<E, S>> row_score;
        std::shared_ptr<ImageBorder<E>> border_left, border_right;

        int32_t row_disparity_max = 0;   // set by prepare()

        Workspace workspace;

        /// v[j] += h[j] for the first n elements
        static void add( S* __restrict v, const S* __restrict h, uint32_t n ) {
            for (uint32_t j = 0; j < n; j++)
                v[j] += h[j];
        }

        /// v[j] -= h[j] for the first n elements
        static void subtract( S* __restrict v, const S* __restrict h, uint32_t n ) {
            for (uint32_t j = 0; j < n; j++)
                v[j] -= h[j];
        }

        /// vertical = the sum of the horizontal scores, added in index order
        void sum_from_scratch( Workspace& ws ) const {
            const uint32_t block = width*disparity_range;
            std::fill_n(ws.vertical.data(), block, (S)0);
            for (uint32_t i = 0; i < region_height; i++)
                add(ws.vertical.data(), ws.horizontal[i].data(), block);
        }

        /// Selects the disparity of every pixel in output row 'row' from the vertical sums, normalized first if the
        /// score requires it
        void select_row( Workspace& ws, DisparitySelect<S>& out, int32_t row ) const {
            if (row_score->require_normalize()) {
                row_score->normalize_region_scores(row, ws.vertical.data(), disparity_min, row_disparity_max, width,
                                                   region_width, region_height, ws.vertical_norm.data());
                out.process(row, ws.vertical_norm.data());
            } else {
                out.process(row, ws.vertical.data());
            }
        }

        /// Scores image row y for every disparity with its border added. See BlockRowScore::score_row()
        void score_image_row( Workspace& ws, int32_t y, S* scores ) const {
            grow_row(*border_left, y, radius_x, ws.left_row.data());
            grow_row(*border_right, y, radius_x, ws.right_row.data());
            row_score->score_row(ws.left_row.data(), ws.right_row.data(), border_left->getWidth(), scores,
                                 disparity_min, row_disparity_max, region_width, ws.element.data());
        }
    };
}

#endif
