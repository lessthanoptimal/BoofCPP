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

// Stereo disparity using block matching. Mirrors BoofCV's DisparityScoreBM_S32 / DisparityScoreBM_F32,
// BlockRowScore and the SelectError / SelectCorrelation classes so that results are identical, including the order
// of floating point operations.
//
// Disparity convention (same as BoofCV): x_right = x_left - disparity. Scores for one image row are stored as
// scores[width*(d - disparity_min) + x], where x is the column in the left image.

namespace boofcv {

    /// Copies image row 'y' plus 'radius' border pixels on each side into 'output', which has width + 2*radius
    /// elements. Rows outside the image are read entirely from the border. Same as BoofCV's GrowBorderSB.growRow().
    template<class E>
    void grow_row( const ImageBorder<E>& border, int32_t y, uint32_t radius, E* output ) {
        const Gray<E>& image = *border.image;
        const int32_t width = image.width;
        const int32_t r = radius;
        if (y < 0 || y >= (int32_t)image.height) {
            for (int32_t i = -r; i < width + r; i++)
                *output++ = border.outside_get(i, y);
        } else {
            for (int32_t i = r; i > 0; i--)
                *output++ = border.outside_get(-i, y);
            std::copy_n(image.data + image.offset + y*image.stride, width, output);
            output += width;
            for (int32_t i = 0; i < r; i++)
                *output++ = border.outside_get(width + i, y);
        }
    }

    /// Computes the score of every region along an image row for each disparity. Same as BoofCV's BlockRowScore.
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

        /// Called with the images before their rows are scored. Same as BoofCV's BlockRowScore.setInput()
        virtual void set_input( const Gray<E>& left, const Gray<E>& right ) {
            (void)left;
            (void)right;
        }

        /// If true then the region scores must be passed through normalize_region_scores()
        virtual bool require_normalize() const { return false; }

        /// Normalization applied to the region scores, the sum of the row scores, before the disparity is selected.
        /// Only scores whose matched column is inside the image are written. Same as BoofCV's
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
        /// Normalizes the scores of 'num_cols' regions in a row for one disparity. Same as BoofCV's
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

    /// Score type for an image type: int32_t for integer images and F32 for floating point images. Same as BoofCV
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

    /// Absolute difference. Same as BoofCV's BlockRowScoreSad
    template<class E>
    struct ErrorSad {
        static BlockScoreType<E> compute( E a, E b ) {
            if constexpr (std::is_floating_point<E>::value)
                return std::abs(a - b);
            else
                return std::abs((int32_t)a - (int32_t)b);
        }
    };

    /// Sum of absolute differences. Same as BoofCV's BlockRowScoreSad
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

    /// Squared difference. Same as BoofCV's BlockRowScoreSsd
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

    /// Sum of squared differences. Same as BoofCV's BlockRowScoreSsd
    template<class E>
    class BlockRowScoreSsd : public BlockRowScoreError<E, BlockScoreType<E>, ErrorSsd<E>> {
    public:
        int32_t max_per_pixel_error() const override {
            return std::is_same<E, U8>::value ? 255*255 : -1;
        }
    };

    /// Zero mean sum of squared differences, computed from the SSD and the local means:
    /// ZSSD = SSD - N*(mean_left - mean_right)^2. Same as BoofCV's BlockRowScoreZssd.F32
    class BlockRowScoreZssd : public BlockRowScoreError<F32, F32, ErrorSsd<F32>> {
    public:
        /// @param border How the local mean reads pixels outside the image
        BlockRowScoreZssd( uint32_t radius_x, uint32_t radius_y, BorderType border ) :
                radius_x(radius_x), radius_y(radius_y), border(FactoryImageBorder::create_SB<F32>(border)) {}

        int32_t max_per_pixel_error() const override { return -1; }

        void set_input( const Gray<F32>& left, const Gray<F32>& right ) override {
            BlurImageOps::mean_border(left, mean_left, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean_border(right, mean_right, radius_x, radius_y, *border, storage, totals);
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

    private:
        int32_t radius_x, radius_y;
        std::shared_ptr<ImageBorder<F32>> border;
        Gray<F32> mean_left{1, 1}, mean_right{1, 1}, storage{1, 1};
        std::vector<F32> totals;
    };

    /// Product of two pixels. Same as BoofCV's BlockRowScoreNcc
    struct ErrorProduct {
        static F32 compute( F32 a, F32 b ) { return a*b; }
    };

    /// Normalized cross correlation, computed from the sum of products and the local mean and standard deviation:
    /// NCC = (sum(L*R)/N - mean_l*mean_r)/(eps + sigma_l*sigma_r). Larger is better. Same as BoofCV's
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
            BlurImageOps::mean_border(tmp, stdev_left, radius_x, radius_y, *border, storage, totals);
            PixelMath::pow2(right, tmp);
            BlurImageOps::mean_border(tmp, stdev_right, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean_border(left, mean_left, radius_x, radius_y, *border, storage, totals);
            BlurImageOps::mean_border(right, mean_right, radius_x, radius_y, *border, storage, totals);
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

    private:
        int32_t radius_x, radius_y;
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

    /// Hamming distance between census transformed images. Same as BoofCV's BlockRowScoreCensus.
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

    /// Selects the best disparity for each pixel in a row from the scores. Same as BoofCV's DisparitySelect.
    template<class S>
    class DisparitySelect {
    public:
        virtual ~DisparitySelect() = default;

        /// @param disparity_max Largest disparity, inclusive
        virtual void configure( int32_t disparity_min, int32_t disparity_max, uint32_t width ) = 0;

        /// Selects the disparity for every pixel in output row 'row'
        virtual void process( uint32_t row, const S* scores ) = 0;
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
    /// running sum. Same as BoofCV's DisparityScoreBM_S32 and DisparityScoreBM_F32. With floating point scores the
    /// running sum is recomputed from scratch every 'catastrophic_reset' rows, at the same rows as BoofCV, so that
    /// the result is identical.
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
                row_score(std::move(row_score)), select(std::move(select)) {}

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

        /// Rows between recomputing the floating point running sum from scratch. Same as BoofCV's
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
            checkSameShape(left, right);

            border_left->setImage(left);
            border_right->setImage(right);
            row_score->set_input(left, right);

            const int32_t width = left.width;
            const int32_t height = left.height;
            const uint32_t block = width*disparity_range;

            horizontal.assign(region_height, std::vector<S>(block, 0));
            vertical.assign(block, 0);
            if (row_score->require_normalize())
                vertical_norm.assign(block, 0);
            element.resize(width + 2*radius_x);
            left_row.resize(width + 2*radius_x);
            right_row.resize(width + 2*radius_x);

            // BoofCV passes min(width, disparity_max) as the largest disparity
            const int32_t row_disparity_max = std::min(width, disparity_max);
            select->configure(disparity_min, disparity_max, width);

            const int32_t row0 = -(int32_t)radius_y;
            const int32_t row1 = height + (int32_t)radius_y;

            // initial block of rows
            for (uint32_t i = 0; i < region_height; i++) {
                score_image_row(row0 + (int32_t)i, horizontal[i].data(), row_disparity_max);
                S* __restrict v = vertical.data();
                const S* __restrict h = horizontal[i].data();
                for (uint32_t j = 0; j < block; j++)
                    v[j] += h[j];
            }
            select_row(0, row_disparity_max, width);

            // remaining rows: subtract the oldest row and add the new one
            for (int32_t row = row0 + (int32_t)region_height; row < row1; row++) {
                S* __restrict v = vertical.data();
                S* __restrict h = horizontal[(row - row0)%region_height].data();
                for (uint32_t j = 0; j < block; j++)
                    v[j] -= h[j];
                score_image_row(row, h, row_disparity_max);
                if (std::is_floating_point<S>::value && (row - (int32_t)radius_y)%catastrophic_reset == 0) {
                    // BoofCV restarts here to limit the floating point error. Same order of addition
                    std::fill(vertical.begin(), vertical.end(), (S)0);
                    for (uint32_t i = 0; i < region_height; i++) {
                        const S* __restrict hi = horizontal[i].data();
                        for (uint32_t j = 0; j < block; j++)
                            v[j] += hi[j];
                    }
                } else {
                    for (uint32_t j = 0; j < block; j++)
                        v[j] += h[j];
                }
                select_row(row - (int32_t)radius_y, row_disparity_max, width);
            }
        }

    private:
        int32_t disparity_min = 0;
        int32_t disparity_max = 0;
        uint32_t disparity_range = 0;

        std::shared_ptr<BlockRowScore<E, S>> row_score;
        std::shared_ptr<DisparitySelect<S>> select;
        std::shared_ptr<ImageBorder<E>> border_left, border_right;
        int32_t catastrophic_reset = 25;

        // horizontal scores of the last region_height rows. vertical is their sum
        std::vector<std::vector<S>> horizontal;
        std::vector<S> vertical;
        // vertical after normalization, if the score requires it
        std::vector<S> vertical_norm;
        std::vector<S> element;
        std::vector<E> left_row, right_row;

        void select_row( int32_t row, int32_t row_disparity_max, int32_t width ) {
            if (row_score->require_normalize()) {
                row_score->normalize_region_scores(row, vertical.data(), disparity_min, row_disparity_max, width,
                                                   region_width, region_height, vertical_norm.data());
                select->process(row, vertical_norm.data());
            } else {
                select->process(row, vertical.data());
            }
        }

        void score_image_row( int32_t y, S* scores, int32_t row_disparity_max ) {
            grow_row(*border_left, y, radius_x, left_row.data());
            grow_row(*border_right, y, radius_x, right_row.data());
            row_score->score_row(left_row.data(), right_row.data(), border_left->getWidth(), scores,
                                 disparity_min, row_disparity_max, region_width, element.data());
        }
    };
}

#endif
