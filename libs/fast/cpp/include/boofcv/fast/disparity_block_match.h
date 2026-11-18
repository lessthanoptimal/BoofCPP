#ifndef BOOFCPP_FAST_DISPARITY_BLOCK_MATCH_H
#define BOOFCPP_FAST_DISPARITY_BLOCK_MATCH_H

#include "boofcv/disparity_block_match.h"

#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace boofcv::fast {
    // Strict's block matching parts
    using boofcv::grow_row;
    using boofcv::BlockScoreType;
    using boofcv::BlockRowScore;
    using boofcv::BlockRowScoreError;
    using boofcv::ErrorSad;
    using boofcv::BlockRowScoreSad;
    using boofcv::ErrorSsd;
    using boofcv::BlockRowScoreSsd;
    using boofcv::BlockRowScoreZssd;
    using boofcv::ErrorProduct;
    using boofcv::BlockRowScoreNcc;
    using boofcv::ErrorCensus;
    using boofcv::BlockRowScoreCensus;
    using boofcv::DisparitySelect;
    using boofcv::DisparityOutput;
    using boofcv::DisparityBlockMatch;
    using boofcv::DisparityBlockMatchRowFormat;

    /// Best disparity of each right image column c, over the left columns c + d + disparity_min inside the image,
    /// searched in increasing order like strict: the first lowest score, or the first largest if LARGEST. Loops
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

    /// Same results as strict's SelectErrorWithChecks. Each loop goes over every column for one disparity at a time,
    /// which is contiguous in the score layout scores[width*(d - disparity_min) + x], so the compiler vectorizes it.
    /// Starting from each column's first disparity and replacing it only with a strictly smaller score selects the
    /// first minimum, the same as strict.
    ///
    /// @tparam S Score type: int32_t, uint16_t or F32. With uint16_t the scores and the disparity range must be below
    /// 65535
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
                        // No candidate is the type's maximum, which is int32_t's in strict
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

    /// Same results as strict's SelectCorrelationWithChecks_F32, with the loops over every column for one disparity
    /// at a time, like SelectErrorWithChecks. The largest score wins, the first one if there are several
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

        /// Largest and smallest score of each column, and the disparity of the largest. Strict updates the worst only
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
        /// worst score like strict
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
}

#endif
