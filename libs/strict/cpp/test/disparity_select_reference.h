#ifndef BOOFCPP_DISPARITY_SELECT_REFERENCE_H
#define BOOFCPP_DISPARITY_SELECT_REFERENCE_H

#include "boofcv/disparity_block_match.h"
#include "boofcv/stereo_disparity.h"

/// The disparity selectors as BoofCV's Java code computes them, one column at a time over its disparities. The
/// reference for strict's selectors in the tests
namespace reference {
    using namespace boofcv;

    /// Winner takes all with optional checks for maximum error, right to left consistency, and texture. Same as
    /// BoofCV's SelectErrorWithChecks_S32 and SelectErrorWithChecks_F32. If the disparity type 'D' is F32 then the
    /// disparity is refined to subpixel precision, same as BoofCV's SelectErrorSubpixel.
    ///
    /// @tparam S Score type, int32_t or F32
    /// @tparam D Disparity image type, U8 or F32
    template<class S, class D>
    class SelectErrorWithChecks : public DisparitySelect<S>, public DisparityOutput<D> {
    public:
        /// Integer texture thresholds are scaled by this. Same as BoofCV
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
            this->disparity_range = disparity_max - disparity_min + 1;
            this->image_width = width;
            invalid_disparity = disparity_range;
            if (std::is_same<D, U8>::value && invalid_disparity > 254)
                throw std::invalid_argument("Max range exceeds maximum value in disparity image");
            column_score.resize(disparity_range);
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

            // Mark all pixels as invalid which can't be estimate due to disparity_min
            for (int32_t col = 0; col < col0; col++)
                out[col] = (D)invalid_disparity;

            for (int32_t col = col0; col < col1; col++) {
                // first valid disparity and the past-the-end disparity (both relative to disparity_min)
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                const int32_t local_range = std::min(col, disparity_max) - disparity_min + 1;

                int32_t index_score = col + local_min*width;
                int32_t best = local_min;
                S score_best = column_score[local_min] = scores[index_score];
                index_score += width;
                for (int32_t d = local_min + 1; d < local_range; d++, index_score += width) {
                    S s = scores[index_score];
                    column_score[d] = s;
                    if (s < score_best) {
                        score_best = s;
                        best = d;
                    }
                }

                // detect bad matches
                if (score_best > max_error_s) {
                    best = invalid_disparity;
                } else if (right_to_left_tolerance >= 0) {
                    // if the association is different going the other direction it is probably noise
                    int32_t disparity_r2l = select_right_to_left(col - best - disparity_min, scores);
                    if (std::abs(disparity_r2l - best) > right_to_left_tolerance)
                        best = invalid_disparity;
                }

                // test to see if the region lacks sufficient texture
                if (texture_threshold > 0 && best != invalid_disparity && local_range - local_min >= 3) {
                    // find the second best disparity value and exclude its neighbors
                    S second_best = std::numeric_limits<S>::max();
                    for (int32_t i = local_min; i < best - 1; i++) {
                        if (column_score[i] < second_best)
                            second_best = column_score[i];
                    }
                    for (int32_t i = best + 2; i < local_range; i++) {
                        if (column_score[i] < second_best)
                            second_best = column_score[i];
                    }

                    if constexpr (std::is_floating_point<S>::value) {
                        if (second_best - score_best <= texture_threshold*score_best)
                            best = invalid_disparity;
                    } else {
                        // 64-bit so it can't overflow. second_best is MAX_VALUE if every candidate was excluded
                        if ((int64_t)DISCRETIZER*(second_best - score_best) <= (int64_t)texture_threshold*score_best)
                            best = invalid_disparity;
                    }
                }

                set_disparity(out, col, best, local_min, local_range);
                if (out_score != nullptr)
                    out_score[col] = (F32)score_best;
            }

            // mark pixels on the upper end as invalid if it can't reach them
            for (int32_t col = col1; col < width; col++)
                out[col] = (D)invalid_disparity;
        }

    private:
        int32_t disparity_min = 0, disparity_max = 0, disparity_range = 0;
        int32_t image_width = 0;
        int32_t invalid_disparity = 0;

        S max_error_s;
        int32_t right_to_left_tolerance;
        S texture_threshold;
        bool squared_error;

        // scores at the current column, indexed by disparity relative to disparity_min
        std::vector<S> column_score;

        void set_disparity( D* out, int32_t col, int32_t value, int32_t local_min, int32_t local_range ) {
            if constexpr (std::is_same<D, F32>::value) {
                if (value <= local_min || value >= local_range - 1) {
                    out[col] = (F32)value;
                } else {
                    // Fit a parabola to the scores around the best one
                    F32 c0 = (F32)column_score[value - 1];
                    F32 c1 = (F32)column_score[value];
                    F32 c2 = (F32)column_score[value + 1];
                    if (!squared_error) {
                        c0 *= c0;
                        c1 *= c1;
                        c2 *= c2;
                    }
                    F32 offset = (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
                    out[col] = (F32)value + offset;
                }
            } else {
                (void)local_min;
                (void)local_range;
                out[col] = (D)value;
            }
        }

        // 'col' is a right image column. Searches disparities whose matched left column (col+d) is inside the image.
        int32_t select_right_to_left( int32_t col, const S* scores ) const {
            const int32_t width = image_width;
            const int32_t local_min = std::max(0, -col - disparity_min);
            const int32_t end = std::min(disparity_range, width - col - disparity_min);

            int32_t index_best = local_min;
            int32_t index_score = (col + disparity_min) + local_min*(width + 1);
            S score_best = scores[index_score];
            for (int32_t d = local_min + 1; d < end; d++) {
                index_score += width + 1;
                S s = scores[index_score];
                if (s < score_best) {
                    score_best = s;
                    index_best = d;
                }
            }
            return index_best;
        }
    };

    template<class D>
    using SelectErrorWithChecks_S32 = SelectErrorWithChecks<int32_t, D>;

    template<class D>
    using SelectErrorWithChecks_F32 = SelectErrorWithChecks<F32, D>;

    /// Selects the disparity with the largest correlation, with optional checks for right to left consistency and
    /// texture. Same as BoofCV's SelectCorrelationWithChecks_F32. If the disparity type 'D' is F32 then the disparity
    /// is refined to subpixel precision, same as BoofCV's SelectCorrelationSubpixel.F32_F32.
    ///
    /// Columns which can't be estimated have a score of NaN. If the texture check is done, the saved score is
    /// relative to the worst score. Both are the same as BoofCV.
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
            this->disparity_range = disparity_max - disparity_min + 1;
            this->image_width = width;
            invalid_disparity = disparity_range;
            if (std::is_same<D, U8>::value && invalid_disparity > 254)
                throw std::invalid_argument("Max range exceeds maximum value in disparity image");
            column_score.resize(disparity_range);
        }

        void process( uint32_t row, const F32* scores ) override {
            Gray<D>* disparity = this->disparity;
            Gray<F32>* score = this->score;
            D* out = disparity->data + disparity->offset + row*disparity->stride;
            F32* out_score = score == nullptr ? nullptr : score->data + score->offset + row*score->stride;
            const int32_t width = image_width;
            const F32 nan = std::numeric_limits<F32>::quiet_NaN();

            // first and last columns it will consider
            const int32_t col0 = std::max(0, disparity_min);
            const int32_t col1 = width + std::min(0, disparity_max);

            for (int32_t col = 0; col < col0; col++)
                set_disparity(out, out_score, col, invalid_disparity, nan, 0, 0);

            for (int32_t col = col0; col < col1; col++) {
                // first valid disparity and the past-the-end disparity (both relative to disparity_min)
                const int32_t local_min = std::max(disparity_min, col - (width - 1)) - disparity_min;
                const int32_t local_range = std::min(col, disparity_max) - disparity_min + 1;

                int32_t index_score = col + local_min*width;
                int32_t best = local_min;
                F32 score_best = column_score[local_min] = scores[index_score];
                F32 score_worst = score_best;
                index_score += width;
                for (int32_t d = local_min + 1; d < local_range; d++, index_score += width) {
                    F32 s = scores[index_score];
                    column_score[d] = s;
                    if (s > score_best) {
                        score_best = s;
                        best = d;
                    } else if (s < score_worst) {
                        score_worst = s;
                    }
                }

                // if the association is different going the other direction it is probably noise
                if (right_to_left_tolerance >= 0) {
                    int32_t disparity_r2l = select_right_to_left(col - best - disparity_min, scores);
                    if (std::abs(disparity_r2l - best) > right_to_left_tolerance)
                        best = invalid_disparity;
                }

                // test to see if the region lacks sufficient texture
                if (texture_threshold > 0 && best != invalid_disparity && local_range - local_min >= 3) {
                    // find the second best disparity value and exclude its neighbors
                    F32 second_best = score_worst;
                    for (int32_t i = local_min; i < best - 1; i++) {
                        if (column_score[i] > second_best)
                            second_best = column_score[i];
                    }
                    for (int32_t i = best + 2; i < local_range; i++) {
                        if (column_score[i] > second_best)
                            second_best = column_score[i];
                    }

                    // relative to the worst score
                    score_best -= score_worst;
                    second_best -= score_worst;

                    if (score_best - second_best <= texture_threshold*second_best)
                        best = invalid_disparity;
                }

                set_disparity(out, out_score, col, best, score_best, local_min, local_range);
            }

            for (int32_t col = col1; col < width; col++)
                set_disparity(out, out_score, col, invalid_disparity, nan, 0, 0);
        }

    private:
        int32_t disparity_min = 0, disparity_max = 0, disparity_range = 0;
        int32_t image_width = 0;
        int32_t invalid_disparity = 0;

        int32_t right_to_left_tolerance;
        F32 texture_threshold;

        // scores at the current column, indexed by disparity relative to disparity_min
        std::vector<F32> column_score;

        void set_disparity( D* out, F32* out_score, int32_t col, int32_t value, F32 score_value,
                            int32_t local_min, int32_t local_range ) {
            if constexpr (std::is_same<D, F32>::value) {
                if (value <= local_min || value >= local_range - 1) {
                    out[col] = (F32)value;
                } else {
                    // Fit a parabola to the scores around the best one
                    F32 c0 = column_score[value - 1];
                    F32 c1 = column_score[value];
                    F32 c2 = column_score[value + 1];
                    F32 offset = (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
                    out[col] = (F32)value + offset;
                }
            } else {
                (void)local_min;
                (void)local_range;
                out[col] = (D)value;
            }
            if (out_score != nullptr)
                out_score[col] = score_value;
        }

        // 'col' is a right image column. Searches disparities whose matched left column (col+d) is inside the image.
        int32_t select_right_to_left( int32_t col, const F32* scores ) const {
            const int32_t width = image_width;
            const int32_t local_min = std::max(0, -col - disparity_min);
            const int32_t end = std::min(disparity_range, width - col - disparity_min);

            int32_t index_best = local_min;
            int32_t index_score = (col + disparity_min) + local_min*(width + 1);
            F32 score_best = scores[index_score];
            for (int32_t d = local_min + 1; d < end; d++) {
                index_score += width + 1;
                F32 s = scores[index_score];
                if (s > score_best) {
                    score_best = s;
                    index_best = d;
                }
            }
            return index_best;
        }
    };

    /// Builds block matching with the selectors above and 32-bit scores
    struct BlockMatchParts {
        template<class S, class D>
        using SelectError = SelectErrorWithChecks<S, D>;

        template<class D>
        using SelectCorrelation = SelectCorrelationWithChecks_F32<D>;

        using SmallScore = int32_t;
    };

    using FactoryStereoDisparity = FactoryBlockMatch<BlockMatchParts>;
}

#endif
