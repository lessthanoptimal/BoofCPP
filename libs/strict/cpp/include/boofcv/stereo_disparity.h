#ifndef BOOFCPP_STEREO_DISPARITY_H
#define BOOFCPP_STEREO_DISPARITY_H

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "boofcv/census_transform.h"
#include "boofcv/config_disparity.h"
#include "boofcv/disparity_block_match.h"
#include "boofcv/image_convert.h"
#include "boofcv/image_misc_ops.h"
#include "boofcv/image_normalization.h"

namespace boofcv {

    /// Stereo disparity using block matching. Owns the output images. BoofCV's WrapBaseBlockMatch.
    ///
    /// DISPARITY IMAGE FORMAT: The value is offset by the minimum disparity. Values >= the disparity range are invalid.
    ///
    /// @tparam E Input image type
    /// @tparam D Disparity image type. U8 for pixel precision, F32 for subpixel
    template<class E, class D>
    class StereoDisparityBlockMatch {
    public:
        virtual ~StereoDisparityBlockMatch() = default;

        /// Computes the disparity between two rectified images and stores it in the internal images
        void process( const Gray<E>& left, const Gray<E>& right ) {
            if (disparity.width != left.width || disparity.height != left.height) {
                // make sure the image borders are marked as invalid
                disparity.reshape(left.width, left.height);
                ImageMiscOps::fill(disparity, (D)get_invalid_value());
            }
            if (score_enabled)
                score.reshape(left.width, left.height);
            process(left, right, disparity, score_enabled ? &score : nullptr);
        }

        /// Computes the disparity and writes it into images owned by the caller, e.g. Java's images. They must have
        /// the same shape as the input. If score is null then the score isn't saved.
        void process( const Gray<E>& left, const Gray<E>& right, Gray<D>& disparity, Gray<F32>* score ) {
            checkSameShape(left, right);
            checkSameShape(left, disparity);
            if (score != nullptr)
                checkSameShape(left, *score);
            select->set_output(disparity, select_saves_score ? score : nullptr);
            process_images(left, right);
        }

        void set_score_enabled( bool enabled ) { score_enabled = enabled; }

        const Gray<D>& get_disparity() const { return disparity; }

        /// Score of the selected disparity, or nullptr if it isn't saved
        const Gray<F32>* get_score() const { return score_enabled ? &score : nullptr; }

        int32_t get_disparity_min() const { return disparity_min; }

        uint32_t get_disparity_range() const { return disparity_range; }

        /// Value of pixels with no valid disparity. Any value >= this is invalid
        int32_t get_invalid_value() const { return (int32_t)get_disparity_range(); }

        int32_t get_border_x() const { return 0; }

        int32_t get_border_y() const { return (int32_t)radius_y; }

    protected:
        /// @param select_saves_score false if the selector never writes the score. Those are the basic winner
        /// takes all selectors, whose score image is allocated but not written.
        StereoDisparityBlockMatch( std::shared_ptr<DisparityOutput<D>> select, bool select_saves_score,
                                   int32_t disparity_min, uint32_t disparity_range, uint32_t radius_y ) :
                select(std::move(select)), select_saves_score(select_saves_score),
                disparity_min(disparity_min), disparity_range(disparity_range), radius_y(radius_y) {}

        /// Computes the disparity. The selector already knows where to write it
        virtual void process_images( const Gray<E>& left, const Gray<E>& right ) = 0;

    private:
        std::shared_ptr<DisparityOutput<D>> select;
        bool select_saves_score;
        int32_t disparity_min;
        uint32_t disparity_range;
        uint32_t radius_y;

        bool score_enabled = false;
        Gray<D> disparity{1, 1};
        Gray<F32> score{1, 1};
    };

    /// Block matching directly on the input images. BoofCV's WrapDisparityBlockMatchRowFormat
    template<class E, class D>
    class StereoDisparityBlockMatchRowFormat : public StereoDisparityBlockMatch<E, D> {
    public:
        StereoDisparityBlockMatchRowFormat( std::unique_ptr<DisparityBlockMatch<E>> alg,
                                            std::shared_ptr<DisparityOutput<D>> select, bool select_saves_score ) :
                StereoDisparityBlockMatch<E, D>(std::move(select), select_saves_score, alg->get_disparity_min(),
                                                alg->get_disparity_range(), alg->get_radius_y()),
                alg(std::move(alg)) {}

    protected:
        void process_images( const Gray<E>& left, const Gray<E>& right ) override {
            alg->process(left, right);
        }

    private:
        std::unique_ptr<DisparityBlockMatch<E>> alg;
    };

    /// Block matching on the census transform of the input images. BoofCV's WrapDisparityBlockMatchCensus
    ///
    /// @tparam C Census image type
    template<class E, class C, class D>
    class StereoDisparityBlockMatchCensus : public StereoDisparityBlockMatch<E, D> {
    public:
        StereoDisparityBlockMatchCensus( std::unique_ptr<FilterCensusTransform<E, C>> census,
                                         std::unique_ptr<DisparityBlockMatch<C>> alg,
                                         std::shared_ptr<DisparityOutput<D>> select,
                                         bool select_saves_score ) :
                StereoDisparityBlockMatch<E, D>(std::move(select), select_saves_score, alg->get_disparity_min(),
                                                alg->get_disparity_range(), alg->get_radius_y()),
                census(std::move(census)), alg(std::move(alg)) {}

        /// Census transform of the left image from the last call to process()
        const Gray<C>& get_census_left() const { return census_left; }

        /// Census transform of the right image from the last call to process()
        const Gray<C>& get_census_right() const { return census_right; }

    protected:
        void process_images( const Gray<E>& left, const Gray<E>& right ) override {
            census->process(left, census_left);
            census->process(right, census_right);
            alg->process(census_left, census_right);
        }

    private:
        std::unique_ptr<FilterCensusTransform<E, C>> census;
        std::unique_ptr<DisparityBlockMatch<C>> alg;
        Gray<C> census_left{1, 1};
        Gray<C> census_right{1, 1};
    };

    /// Converts the input images to F32, optionally normalizing them, then block matching on F32 images.
    /// BoofCV's DisparityBlockMatchPreFilter
    struct BlockMatchParts;

    /// @tparam Parts Its Normalization and convert() prepare the input, see BlockMatchParts
    template<class E, class D, class Parts = BlockMatchParts>
    class StereoDisparityBlockMatchPreFilter : public StereoDisparityBlockMatch<E, D> {
    public:
        /// @param normalize If true then the left image is normalized to zero mean and a maximum absolute value of
        /// one, and the right image with the same parameters. BoofCV's Type.CORRELATION. Otherwise the images are
        /// only converted, BoofCV's Type.NONE.
        StereoDisparityBlockMatchPreFilter( std::unique_ptr<DisparityBlockMatch<F32>> alg,
                                            std::shared_ptr<DisparityOutput<D>> select,
                                            bool select_saves_score, bool normalize ) :
                StereoDisparityBlockMatch<E, D>(std::move(select), select_saves_score, alg->get_disparity_min(),
                                                alg->get_disparity_range(), alg->get_radius_y()),
                alg(std::move(alg)), normalize(normalize) {}

        /// Left image after conversion from the last call to process()
        const Gray<F32>& get_adjusted_left() const { return adjusted_left; }

        /// Right image after conversion from the last call to process()
        const Gray<F32>& get_adjusted_right() const { return adjusted_right; }

        /// Normalization applied by the last call to process(), if normalize is on
        const NormalizeParameters& get_parameters() const { return parameters; }

    protected:
        void process_images( const Gray<E>& left, const Gray<E>& right ) override {
            if (normalize) {
                Parts::Normalization::zero_mean_max_one(left, adjusted_left, parameters);
                // The same parameters for both images since the cameras' gain and exposure are assumed to be the same
                Parts::Normalization::apply(right, parameters, adjusted_right);
            } else {
                Parts::convert(left, adjusted_left);
                Parts::convert(right, adjusted_right);
            }
            alg->process(adjusted_left, adjusted_right);
        }

    private:
        std::unique_ptr<DisparityBlockMatch<F32>> alg;
        bool normalize;
        NormalizeParameters parameters;
        Gray<F32> adjusted_left{1, 1};
        Gray<F32> adjusted_right{1, 1};
    };

    /// Creates census transforms that use 'Census', CensusTransform or CensusTransform_MT. BoofCV's
    /// FactoryCensusTransform
    template<class Census>
    class FactoryCensus {
    public:
        /// Throws std::invalid_argument if the variant doesn't produce a 'C' image
        ///
        /// @param border How the census transform reads pixels outside the image. SKIP if the image border isn't
        /// processed. NORMALIZED isn't supported.
        /// @tparam C Census image type of the variant. U8 for BLOCK_3_3, S32 for BLOCK_5_5 and S64 for the others
        template<class E, class C>
        static std::unique_ptr<FilterCensusTransform<E, C>> variant( CensusVariants type, BorderType border ) {
            if (border == NORMALIZED)
                throw std::invalid_argument("Normalized border isn't supported by census transforms");
            std::shared_ptr<ImageBorder<E>> image_border;
            if (border != SKIP)
                image_border = FactoryImageBorder::create_SB<E>(border);
            switch (type) {
                case CensusVariants::BLOCK_3_3:
                    if constexpr (std::is_same<C, U8>::value)
                        return std::make_unique<FilterCensusTransformD33U8<E, Census>>(image_border);
                    break;

                case CensusVariants::BLOCK_5_5:
                    if constexpr (std::is_same<C, S32>::value)
                        return std::make_unique<FilterCensusTransformD55S32<E, Census>>(image_border);
                    break;

                default:
                    if constexpr (std::is_same<C, S64>::value)
                        return std::make_unique<FilterCensusTransformSampleS64<E, Census>>(samples(type),
                                                                                                    image_border);
                    break;
            }
            throw std::invalid_argument("Census image type doesn't match the variant");
        }

        /// Points sampled by a variant that produces an S64 census image
        static std::vector<Point2D<int32_t>> samples( CensusVariants type ) {
            switch (type) {
                case CensusVariants::BLOCK_7_7: return CensusTransform::create_block_samples(3, 3);
                case CensusVariants::BLOCK_9_7: return CensusTransform::create_block_samples(4, 3);
                case CensusVariants::BLOCK_13_5: return CensusTransform::create_block_samples(6, 2);
                case CensusVariants::CIRCLE_9: return CensusTransform::create_circle_samples();
                default: throw std::invalid_argument("Census variant doesn't produce an S64 image");
            }
        }
    };

    /// Single threaded census transforms
    using FactoryCensusTransform = FactoryCensus<CensusTransform>;

    /// The single threaded parts FactoryBlockMatch builds block matching from that a variant can replace
    struct BlockMatchParts {
        /// Selects the disparity from error scores
        template<class S, class D>
        using SelectError = SelectErrorWithChecks<S, D>;

        /// Selects the disparity from correlation scores
        template<class D>
        using SelectCorrelation = SelectCorrelationWithChecks_F32<D>;

        /// Integer score type which replaces int32_t when the largest region score and the disparity range are
        /// below its maximum. int32_t if there's none. The running sums of 16-bit scores are exact modulo 2^16, so
        /// region scores below 65535 are exact
        using SmallScore = uint16_t;

        /// Block matching on the row scores
        template<class E, class S>
        using RowFormat = DisparityBlockMatchRowFormat<E, S>;

        using RowScoreZssd = BlockRowScoreZssd;
        using RowScoreNcc = BlockRowScoreNcc;

        /// The census transform, CensusTransform or CensusTransform_MT
        using Census = CensusTransform;

        /// Normalizes the input of NCC
        using Normalization = ImageNormalization;

        /// Converts the input of ZSSD, and NCC's if it isn't normalized
        template<class I, class O>
        static void convert( const Gray<I>& input, Gray<O>& output ) { boofcv::convert(input, output); }
    };

    /// Creates stereo disparity algorithms from 'Parts'. BoofCV's FactoryStereoDisparity
    ///
    /// @tparam Parts Variant's parts, see BlockMatchParts
    template<class Parts>
    class FactoryBlockMatch {
    public:
        /// Block matching. Throws std::invalid_argument if the configuration isn't supported.
        ///
        /// The border in the configuration is used by block matching, the census transform, and the local statistics
        /// of ZSSD and NCC.
        ///
        /// @tparam E Input image type. U8 or F32
        /// @tparam D Disparity image type. F32 if subpixel is on, otherwise U8
        template<class E, class D>
        static std::unique_ptr<StereoDisparityBlockMatch<E, D>> block_match( const ConfigDisparityBM& config ) {
            config.checkValidity();

            if (config.subpixel && !std::is_same<D, F32>::value)
                throw std::invalid_argument("With subpixel on, disparity image must be F32");
            if (!config.subpixel && !std::is_same<D, U8>::value)
                throw std::invalid_argument("With subpixel off, disparity image must be U8");
            if (!std::is_same<E, U8>::value && !std::is_same<E, F32>::value)
                throw std::invalid_argument("Only U8 and F32 images are supported");

            std::unique_ptr<StereoDisparityBlockMatch<E, D>> ret;
            switch (config.error_type) {
                case DisparityError::SAD:
                    if constexpr (std::is_same<E, U8>::value) {
                        if (small_scores(config, 255)) {
                            using S = typename Parts::SmallScore;
                            ret = create_row_format<E, D, S>(config, std::make_shared<BlockRowScoreSad<E, S>>());
                            break;
                        }
                    }
                    ret = create_row_format<E, D, BlockScoreType<E>>(config, std::make_shared<BlockRowScoreSad<E>>());
                    break;

                case DisparityError::SSD:
                    ret = create_row_format<E, D, BlockScoreType<E>>(config, std::make_shared<BlockRowScoreSsd<E>>());
                    break;

                case DisparityError::ZSSD:
                    // Not normalized, unlike NCC, since rescaling the input would change the scale of the error
                    ret = create_pre_filter<E, D>(config, std::make_shared<typename Parts::RowScoreZssd>(
                            config.region_radius_x, config.region_radius_y, config.border), false);
                    break;

                case DisparityError::NCC:
                    ret = create_pre_filter<E, D>(config, std::make_shared<typename Parts::RowScoreNcc>(
                            config.region_radius_x, config.region_radius_y, config.border,
                            (F32)config.config_ncc.eps), config.config_ncc.normalize_input);
                    break;

                case DisparityError::CENSUS: {
                    const CensusVariants variant = config.config_census.variant;
                    if (variant == CensusVariants::BLOCK_3_3) {
                        ret = create_census<E, U8, D>(config);
                    } else if (variant == CensusVariants::BLOCK_5_5) {
                        ret = create_census<E, S32, D>(config);
                    } else {
                        ret = create_census<E, S64, D>(config);
                    }
                } break;

                default:
                    throw std::invalid_argument("Unknown error type");
            }
            ret->set_score_enabled(config.save_score);
            return ret;
        }

        /// True if Parts::SmallScore can replace int32_t: the largest region score, the region's area times
        /// 'max_per_pixel', and the disparity range are below its maximum
        static bool small_scores( const ConfigDisparityBM& config, int32_t max_per_pixel ) {
            using S = typename Parts::SmallScore;
            if (std::is_same<S, int32_t>::value)
                return false;
            const int64_t area = (int64_t)(config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1);
            const int64_t limit = std::numeric_limits<S>::max();
            return area*max_per_pixel < limit && config.disparity_range < limit;
        }

    private:
        /// A selector, both as what the block matcher passes scores to and as what the wrapper sets the output of
        template<class S, class D>
        struct Selector {
            std::shared_ptr<DisparitySelect<S>> scores;
            std::shared_ptr<DisparityOutput<D>> output;
            // false for the basic winner takes all selectors, which don't write the score
            bool saves_score;
        };

        /// BoofCV's FactoryStereoDisparity.createDisparitySelect(). With every check off and no subpixel,
        /// BoofCV uses a selector that doesn't save the score. The selector here selects the same disparity.
        template<class S, class D>
        static Selector<S, D> create_select( const ConfigDisparityBM& config ) {
            const double max_error = (config.region_radius_x*2 + 1)*(config.region_radius_y*2 + 1)*
                                     config.max_per_pixel_error;
            const bool checks_off = config.validate_r_to_l < 0 && config.texture <= 0;
            const bool squared = config.error_type == DisparityError::SSD || config.error_type == DisparityError::ZSSD;
            Selector<S, D> ret;
            if (config.error_type == DisparityError::NCC) {
                if constexpr (std::is_same<S, F32>::value) {
                    auto select = std::make_shared<typename Parts::template SelectCorrelation<D>>(
                            config.validate_r_to_l, config.texture);
                    ret.scores = select;
                    ret.output = select;
                    ret.saves_score = config.subpixel || !checks_off;
                } else {
                    throw std::invalid_argument("Can't do correlation scores for integer image types");
                }
            } else {
                auto select = std::make_shared<typename Parts::template SelectError<S, D>>(
                        max_error, config.validate_r_to_l, config.texture, squared);
                ret.scores = select;
                ret.output = select;
                ret.saves_score = config.subpixel || max_error >= 0 || !checks_off;
            }
            return ret;
        }

        template<class E, class S>
        static std::unique_ptr<DisparityBlockMatchRowFormat<E, S>> create_block_matching(
                const ConfigDisparityBM& config, std::shared_ptr<BlockRowScore<E, S>> row_score,
                std::shared_ptr<DisparitySelect<S>> select ) {
            auto alg = std::make_unique<typename Parts::template RowFormat<E, S>>(
                    config.region_radius_x, config.region_radius_y, std::move(row_score), std::move(select));
            alg->configure(config.disparity_min, config.disparity_range);
            alg->set_catastrophic_reset(config.catastrophic_reset);
            alg->set_border(config.border);
            return alg;
        }

        template<class E, class D, class S>
        static std::unique_ptr<StereoDisparityBlockMatch<E, D>> create_row_format(
                const ConfigDisparityBM& config, std::shared_ptr<BlockRowScore<E, S>> row_score ) {
            auto select = create_select<S, D>(config);
            auto alg = create_block_matching<E, S>(config, std::move(row_score), select.scores);
            return std::make_unique<StereoDisparityBlockMatchRowFormat<E, D>>(
                    std::move(alg), select.output, select.saves_score);
        }

        template<class E, class D>
        static std::unique_ptr<StereoDisparityBlockMatch<E, D>> create_pre_filter(
                const ConfigDisparityBM& config, std::shared_ptr<BlockRowScore<F32, F32>> row_score, bool normalize ) {
            auto select = create_select<F32, D>(config);
            auto alg = create_block_matching<F32, F32>(config, std::move(row_score), select.scores);
            return std::make_unique<StereoDisparityBlockMatchPreFilter<E, D, Parts>>(
                    std::move(alg), select.output, select.saves_score, normalize);
        }

        template<class E, class C, class D>
        static std::unique_ptr<StereoDisparityBlockMatch<E, D>> create_census( const ConfigDisparityBM& config ) {
            // Census always has an integer score, even with a floating point input image
            const int32_t bits = census_bits(config.config_census.variant);
            if (small_scores(config, bits))
                return create_census<E, C, D, typename Parts::SmallScore>(config, bits);
            return create_census<E, C, D, int32_t>(config, bits);
        }

        template<class E, class C, class D, class S>
        static std::unique_ptr<StereoDisparityBlockMatch<E, D>> create_census( const ConfigDisparityBM& config,
                                                                               int32_t bits ) {
            auto select = create_select<S, D>(config);
            auto census = FactoryCensus<typename Parts::Census>::template variant<E, C>(config.config_census.variant,
                                                                                        config.border);
            auto alg = create_block_matching<C, S>(config, std::make_shared<BlockRowScoreCensus<C, S>>(bits),
                                                   select.scores);
            return std::make_unique<StereoDisparityBlockMatchCensus<E, C, D>>(
                    std::move(census), std::move(alg), select.output, select.saves_score);
        }
    };

    /// Strict's single threaded block matching
    using FactoryStereoDisparity = FactoryBlockMatch<BlockMatchParts>;
}

#endif
