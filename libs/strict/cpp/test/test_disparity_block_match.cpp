#include "gtest/gtest.h"
#include "boofcv/disparity_block_match.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <cmath>
#include <limits>
#include <random>
#include <tuple>

using namespace std;
using namespace boofcv;

namespace {
    /// Brute force score for a region, reading pixels through the border
    int32_t naive_sad( const ImageBorder<U8>& L, const ImageBorder<U8>& R, int32_t x, int32_t y, int32_t d,
                       int32_t rx, int32_t ry ) {
        int32_t sum = 0;
        for (int32_t j = -ry; j <= ry; j++) {
            for (int32_t i = -rx; i <= rx; i++) {
                sum += std::abs((int32_t)L.get(x + i, y + j) - (int32_t)R.get(x - d + i, y + j));
            }
        }
        return sum;
    }

    /// Brute force reference: score of every pixel and disparity, then select with the same rules as BoofCV
    struct NaiveBlockMatch {
        int32_t rx, ry, dmin, dmax;
        double max_error;
        int32_t r2l;
        double texture;
        BorderType border_type;

        int32_t width = 0;
        // volume[(y*width + x)*range + (d-dmin)], -1 if not defined
        vector<int32_t> volume;

        int32_t range() const { return dmax - dmin + 1; }

        int32_t at( int32_t x, int32_t y, int32_t d ) const { return volume[(y*width + x)*range() + d - dmin]; }

        void compute( const Gray<U8>& left, const Gray<U8>& right ) {
            auto L = FactoryImageBorder::create_SB<U8>(border_type);
            auto R = FactoryImageBorder::create_SB<U8>(border_type);
            L->setImage(left);
            R->setImage(right);
            width = left.width;
            volume.assign(left.width*left.height*range(), -1);
            for (int32_t y = 0; y < (int32_t)left.height; y++) {
                for (int32_t x = 0; x < width; x++) {
                    for (int32_t d = dmin; d <= dmax; d++) {
                        if (x - d < 0 || x - d >= width)
                            continue;
                        volume[(y*width + x)*range() + d - dmin] = naive_sad(*L, *R, x, y, d, rx, ry);
                    }
                }
            }
        }

        /// Returns the selected disparity relative to dmin, or range() if invalid. Optionally the subpixel value
        int32_t select( int32_t x, int32_t y, int32_t& best_score, float* subpixel ) const {
            const int32_t invalid = range();
            const int32_t d0 = std::max(dmin, x - (width - 1));
            const int32_t d1 = std::min(x, dmax);
            if (d0 > d1) {
                best_score = -1;
                if (subpixel != nullptr)
                    *subpixel = (float)invalid;
                return invalid;
            }

            int32_t best = d0;
            for (int32_t d = d0 + 1; d <= d1; d++) {
                if (at(x, y, d) < at(x, y, best))
                    best = d;
            }
            best_score = at(x, y, best);

            int32_t result = best - dmin;
            int32_t max_i = max_error <= 0 ? std::numeric_limits<int32_t>::max() : (int32_t)max_error;
            if (best_score > max_i) {
                result = invalid;
            } else if (r2l >= 0) {
                // right image column, search every disparity whose left column is inside the image
                int32_t xr = x - best;
                int32_t best_r = 0;
                bool found = false;
                for (int32_t d = dmin; d <= dmax; d++) {
                    if (xr + d < 0 || xr + d >= width)
                        continue;
                    if (!found || at(xr + d, y, d) < at(xr + best_r, y, best_r))
                        best_r = d;
                    found = true;
                }
                if (std::abs(best_r - best) > r2l)
                    result = invalid;
            }

            int32_t threshold = (int32_t)(10000*texture);
            if (threshold > 0 && result != invalid && d1 - d0 + 1 >= 3) {
                int64_t second = std::numeric_limits<int32_t>::max();
                for (int32_t d = d0; d <= d1; d++) {
                    if (std::abs(d - best) <= 1)
                        continue;
                    second = std::min<int64_t>(second, at(x, y, d));
                }
                // 64-bit like BoofCV, so a pixel without any competing disparity is kept
                if (10000*(second - best_score) <= (int64_t)threshold*best_score)
                    result = invalid;
            }

            if (subpixel != nullptr) {
                if (result == invalid || best - 1 < d0 || best + 1 > d1) {
                    *subpixel = (float)result;
                } else {
                    float c0 = (float)at(x, y, best - 1), c1 = (float)at(x, y, best), c2 = (float)at(x, y, best + 1);
                    c0 *= c0;
                    c1 *= c1;
                    c2 *= c2;
                    *subpixel = (float)result + (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
                }
            }
            return result;
        }
    };

    struct Config {
        uint32_t width, height;
        int32_t rx, ry, dmin, range;
        double max_error;
        int32_t r2l;
        double texture;
        BorderType border;
    };

    template<class D>
    void compare_to_naive( const Config& c, bool save_score ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(left, right, c.width, c.height, 4);

        auto select = make_shared<SelectErrorWithChecks_S32<D>>(c.max_error, c.r2l, c.texture);
        DisparityBlockMatchRowFormat<U8, int32_t> alg(c.rx, c.ry, make_shared<BlockRowScoreSad<U8>>(), select);
        alg.configure(c.dmin, c.range);
        alg.set_border(c.border);

        Gray<D> disparity(c.width, c.height);
        Gray<F32> score(c.width, c.height);
        ImageMiscOps::fill(score, -2.0f);
        select->set_output(disparity, save_score ? &score : nullptr);
        alg.process(left, right);

        NaiveBlockMatch naive{c.rx, c.ry, c.dmin, c.dmin + c.range - 1, c.max_error, c.r2l, c.texture, c.border};
        naive.compute(left, right);

        uint32_t valid = 0;
        for (int32_t y = 0; y < (int32_t)c.height; y++) {
            for (int32_t x = 0; x < (int32_t)c.width; x++) {
                int32_t best_score;
                float subpixel;
                int32_t expected = naive.select(x, y, best_score, &subpixel);
                if (std::is_same<D, F32>::value) {
                    ASSERT_EQ(subpixel, (float)disparity.at(x, y)) << x << " " << y << " config dmin=" << c.dmin << " range=" << c.range;
                } else {
                    ASSERT_EQ(expected, (int32_t)disparity.at(x, y)) << x << " " << y;
                }
                // Score is only written where a disparity could be estimated
                float expected_score = !save_score || best_score < 0 ? -2.0f : (float)best_score;
                ASSERT_EQ(expected_score, score.at(x, y)) << x << " " << y;
                if (expected != naive.range())
                    valid++;
            }
        }
        // Make sure the test isn't trivial
        ASSERT_GT(valid, 0u);
    }

    const vector<Config> CONFIGS = {
            // w, h, rx, ry, dmin, range, max_error, r2l, texture, border
            {30, 25, 2, 2, 0, 10, 0, -1, 0, REFLECT},     // plain winner takes all
            {30, 25, 3, 1, 0, 10, 0, -1, 0, EXTENDED},    // rx != ry
            {30, 25, 1, 3, 2, 7, 0, -1, 0, WRAP},         // positive minimum
            {30, 25, 2, 2, -5, 12, 0, -1, 0, ZERO},       // negative disparities
            {30, 25, 2, 1, -8, 6, 0, -1, 0, REFLECT},     // only negative disparities
            {12, 9, 2, 2, 0, 20, 0, -1, 0, REFLECT},      // range larger than the image
            {30, 25, 2, 2, 0, 10, 400, -1, 0, REFLECT},   // max error
            {30, 25, 2, 2, 0, 10, 0, 1, 0, REFLECT},      // right to left
            {30, 25, 2, 2, -5, 12, 0, 0, 0, REFLECT},     // right to left, negative disparities
            {30, 25, 2, 2, 0, 10, 0, -1, 0.15, REFLECT},  // texture
            {30, 25, 2, 2, 0, 3, 0, -1, 0.15, REFLECT},   // texture, all candidates excluded at range 3
            {31, 23, 3, 2, -3, 15, 600, 1, 0.15, EXTENDED} // everything, odd sizes
    };
}

TEST(GrowRow, compare_to_border) {
    Gray<U8> image(7, 5);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(image, (U8)0, (U8)255, gen);
    for (BorderType type : {EXTENDED, REFLECT, WRAP, ZERO}) {
        auto border = FactoryImageBorder::create_SB<U8>(type);
        border->setImage(image);
        const uint32_t r = 3;
        vector<U8> row(image.width + 2*r);
        for (int32_t y = -3; y < 8; y++) {
            grow_row(*border, y, r, row.data());
            for (int32_t x = -(int32_t)r; x < (int32_t)(image.width + r); x++) {
                ASSERT_EQ(border->get(x, y), row[x + r]) << type << " " << x << " " << y;
            }
        }
    }
}

TEST(BlockRowScoreSad, compare_to_naive) {
    const uint32_t width = 20, r = 2, region = 2*r + 1;
    Gray<U8> left(width, 1), right(width, 1);
    std::mt19937 gen(0xBEEF);
    ImageMiscOps::fill_uniform(left, (U8)0, (U8)255, gen);
    ImageMiscOps::fill_uniform(right, (U8)0, (U8)255, gen);
    auto bl = FactoryImageBorder::create_SB<U8>(REFLECT);
    auto br = FactoryImageBorder::create_SB<U8>(REFLECT);
    bl->setImage(left);
    br->setImage(right);
    vector<U8> lrow(width + 2*r), rrow(width + 2*r);
    grow_row(*bl, 0, r, lrow.data());
    grow_row(*br, 0, r, rrow.data());

    BlockRowScoreSad<U8> alg;
    EXPECT_EQ(255, alg.max_per_pixel_error());
    for (int32_t dmin : {0, 3, -4}) {
        const int32_t dmax = dmin + 8;
        vector<int32_t> scores(width*(dmax - dmin + 1), -1);
        vector<int32_t> element(width + 2*r);
        alg.score_row(lrow.data(), rrow.data(), width, scores.data(), dmin, dmax, region, element.data());
        for (int32_t d = dmin; d <= dmax; d++) {
            for (int32_t x = 0; x < (int32_t)width; x++) {
                int32_t found = scores[width*(d - dmin) + x];
                if (x - d < 0 || x - d >= (int32_t)width) {
                    // not touched
                    ASSERT_EQ(-1, found);
                    continue;
                }
                int32_t expected = 0;
                for (int32_t i = -(int32_t)r; i <= (int32_t)r; i++)
                    expected += std::abs((int32_t)bl->get(x + i, 0) - (int32_t)br->get(x - d + i, 0));
                ASSERT_EQ(expected, found) << dmin << " " << d << " " << x;
            }
        }
    }
}

namespace {
    /// Census scores against a brute force hamming distance, for every disparity
    template<class E>
    void census_row_compare_to_naive( E max_value ) {
        const uint32_t width = 20, r = 2, region = 2*r + 1;
        Gray<E> left(width, 1), right(width, 1);
        std::mt19937 gen(0xBEEF);
        if constexpr (std::is_same<E, S64>::value) {
            // every bit, including the sign bit
            for (Gray<E>* image : {&left, &right})
                for (uint32_t x = 0; x < width; x++)
                    image->at(x, 0) = (S64)(((uint64_t)gen() << 32) | gen());
        } else {
            ImageMiscOps::fill_uniform(left, (E)0, max_value, gen);
            ImageMiscOps::fill_uniform(right, (E)0, max_value, gen);
        }
        auto bl = FactoryImageBorder::create_SB<E>(REFLECT);
        auto br = FactoryImageBorder::create_SB<E>(REFLECT);
        bl->setImage(left);
        br->setImage(right);
        vector<E> lrow(width + 2*r), rrow(width + 2*r);
        grow_row(*bl, 0, r, lrow.data());
        grow_row(*br, 0, r, rrow.data());

        BlockRowScoreCensus<E> alg(24);
        EXPECT_EQ(24, alg.max_per_pixel_error());
        for (int32_t dmin : {0, 3, -4}) {
            const int32_t dmax = dmin + 8;
            vector<int32_t> scores(width*(dmax - dmin + 1), -1);
            vector<int32_t> element(width + 2*r);
            alg.score_row(lrow.data(), rrow.data(), width, scores.data(), dmin, dmax, region, element.data());
            for (int32_t d = dmin; d <= dmax; d++) {
                for (int32_t x = 0; x < (int32_t)width; x++) {
                    int32_t found = scores[width*(d - dmin) + x];
                    if (x - d < 0 || x - d >= (int32_t)width) {
                        ASSERT_EQ(-1, found);
                        continue;
                    }
                    int32_t expected = 0;
                    for (int32_t i = -(int32_t)r; i <= (int32_t)r; i++) {
                        uint64_t diff = (uint64_t)(int64_t)bl->get(x + i, 0) ^ (uint64_t)(int64_t)br->get(x - d + i, 0);
                        for (int b = 0; b < 64; b++)
                            expected += (diff >> b) & 1;
                    }
                    ASSERT_EQ(expected, found) << dmin << " " << d << " " << x;
                }
            }
        }
    }
}

TEST(BlockRowScoreCensus, compare_to_naive) {
    census_row_compare_to_naive<U8>((U8)255);
    census_row_compare_to_naive<S32>((S32)0xFFFFFF);
    census_row_compare_to_naive<S64>((S64)0);
}

TEST(DisparityBlockMatchRowFormat, compare_to_naive_U8) {
    for (const auto& c : CONFIGS) {
        compare_to_naive<U8>(c, true);
        compare_to_naive<U8>(c, false);
    }
}

TEST(DisparityBlockMatchRowFormat, compare_to_naive_subpixel) {
    for (const auto& c : CONFIGS) {
        compare_to_naive<F32>(c, true);
    }
}

TEST(DisparityBlockMatchRowFormat, run_twice_same_result) {
    Gray<U8> left(1, 1), right(1, 1);
    create_stereo_pair(left, right, 30, 25, 4);
    auto select = make_shared<SelectErrorWithChecks_S32<F32>>(0, 1, 0.15);
    DisparityBlockMatchRowFormat<U8, int32_t> alg(2, 2, make_shared<BlockRowScoreSad<U8>>(), select);
    alg.configure(-3, 12);
    alg.set_border(REFLECT);

    Gray<F32> first(30, 25), second(30, 25), score(30, 25);
    select->set_output(first, &score);
    alg.process(left, right);
    select->set_output(second, &score);
    alg.process(left, right);
    for (uint32_t i = 0; i < first.data_length; i++)
        ASSERT_EQ(first.data[i], second.data[i]);
}

TEST(SelectErrorWithChecks_S32, range_too_large_for_U8) {
    SelectErrorWithChecks_S32<U8> select(0, -1, 0);
    EXPECT_THROW(select.configure(0, 254, 30), std::invalid_argument);
    EXPECT_NO_THROW(select.configure(0, 253, 30));
}

TEST(DisparityBlockMatchRowFormat, bad_input) {
    auto select = make_shared<SelectErrorWithChecks_S32<U8>>(0, -1, 0);
    DisparityBlockMatchRowFormat<U8, int32_t> alg(2, 2, make_shared<BlockRowScoreSad<U8>>(), select);
    EXPECT_THROW(alg.configure(0, 0), std::invalid_argument);
    alg.configure(0, 5);
    alg.set_border(REFLECT);
    Gray<U8> a(10, 8), b(10, 9);
    Gray<U8> disparity(10, 8);
    select->set_output(disparity, nullptr);
    EXPECT_THROW(alg.process(a, b), std::invalid_argument);
}

namespace {
    /// Row scores against a brute force sum of the per pixel error, for every disparity
    template<class E, class Alg, class Error>
    void row_compare_to_naive( const Alg& alg, Error error, E max_value ) {
        using S = BlockScoreType<E>;
        const uint32_t width = 20, r = 2, region = 2*r + 1;
        Gray<E> left(width, 1), right(width, 1);
        std::mt19937 gen(0xBEEF);
        ImageMiscOps::fill_uniform(left, (E)0, max_value, gen);
        ImageMiscOps::fill_uniform(right, (E)0, max_value, gen);
        auto bl = FactoryImageBorder::create_SB<E>(REFLECT);
        auto br = FactoryImageBorder::create_SB<E>(REFLECT);
        bl->setImage(left);
        br->setImage(right);
        vector<E> lrow(width + 2*r), rrow(width + 2*r);
        grow_row(*bl, 0, r, lrow.data());
        grow_row(*br, 0, r, rrow.data());

        for (int32_t dmin : {0, 3, -4}) {
            const int32_t dmax = dmin + 8;
            vector<S> scores(width*(dmax - dmin + 1), -1);
            vector<S> element(width + 2*r);
            alg.score_row(lrow.data(), rrow.data(), width, scores.data(), dmin, dmax, region, element.data());
            for (int32_t d = dmin; d <= dmax; d++) {
                for (int32_t x = 0; x < (int32_t)width; x++) {
                    S found = scores[width*(d - dmin) + x];
                    if (x - d < 0 || x - d >= (int32_t)width) {
                        ASSERT_EQ((S)-1, found);
                        continue;
                    }
                    double expected = 0;
                    for (int32_t i = -(int32_t)r; i <= (int32_t)r; i++)
                        expected += error((double)bl->get(x + i, 0), (double)br->get(x - d + i, 0));
                    ASSERT_NEAR(expected, (double)found, 1e-5*std::max(1.0, expected)) << dmin << " " << d << " " << x;
                }
            }
        }
    }
}

TEST(BlockRowScoreSad, compare_to_naive_F32) {
    BlockRowScoreSad<F32> alg;
    EXPECT_EQ(-1, alg.max_per_pixel_error());
    row_compare_to_naive<F32>(alg, []( double a, double b ) { return std::abs(a - b); }, 1.0f);
}

TEST(BlockRowScoreSsd, compare_to_naive) {
    BlockRowScoreSsd<U8> alg_u8;
    EXPECT_EQ(255*255, alg_u8.max_per_pixel_error());
    row_compare_to_naive<U8>(alg_u8, []( double a, double b ) { return (a - b)*(a - b); }, (U8)255);
    BlockRowScoreSsd<F32> alg_f32;
    EXPECT_EQ(-1, alg_f32.max_per_pixel_error());
    row_compare_to_naive<F32>(alg_f32, []( double a, double b ) { return (a - b)*(a - b); }, 1.0f);
}

namespace {
    /// Saves the region scores of every row it's given
    struct CaptureSelect : public DisparitySelect<F32> {
        int32_t dmin = 0, dmax = 0;
        uint32_t width = 0;
        vector<vector<F32>> rows;

        void configure( int32_t disparity_min, int32_t disparity_max, uint32_t width ) override {
            dmin = disparity_min;
            dmax = disparity_max;
            this->width = width;
            rows.clear();
        }

        void process( uint32_t row, const F32* scores ) override {
            if (rows.size() <= row)
                rows.resize(row + 1);
            rows[row].assign(scores, scores + width*(dmax - dmin + 1));
        }
    };

    /// Statistics of a region read through a border, in double
    struct RegionStats {
        double mean_l = 0, mean_r = 0, mean_ll = 0, mean_rr = 0, mean_lr = 0, ssd = 0;
    };

    RegionStats region_stats( const ImageBorder<F32>& L, const ImageBorder<F32>& R, int32_t x, int32_t y, int32_t d,
                              int32_t rx, int32_t ry ) {
        RegionStats s;
        const double n = (2*rx + 1)*(2*ry + 1);
        for (int32_t j = -ry; j <= ry; j++) {
            for (int32_t i = -rx; i <= rx; i++) {
                double l = L.get(x + i, y + j), r = R.get(x - d + i, y + j);
                s.mean_l += l/n;
                s.mean_r += r/n;
                s.mean_ll += l*l/n;
                s.mean_rr += r*r/n;
                s.mean_lr += l*r/n;
                s.ssd += (l - r)*(l - r);
            }
        }
        return s;
    }

    /// Runs block matching with ZSSD or NCC and compares the normalized region scores to brute force
    template<class RowScore>
    void normalized_compare_to_naive( bool ncc, uint32_t width, uint32_t height, int32_t rx, int32_t ry,
                                      int32_t dmin, int32_t range, BorderType border, int32_t reset ) {
        Gray<F32> left(width, height), right(width, height);
        std::mt19937 gen(0xBEEF + width + rx);
        ImageMiscOps::fill_uniform(left, 0.0f, 1.0f, gen);
        ImageMiscOps::fill_uniform(right, 0.0f, 1.0f, gen);
        const F32 eps = 1e-4f;

        std::shared_ptr<BlockRowScore<F32, F32>> row_score;
        if constexpr (std::is_same<RowScore, BlockRowScoreNcc>::value)
            row_score = make_shared<BlockRowScoreNcc>(rx, ry, border, eps);
        else
            row_score = make_shared<BlockRowScoreZssd>(rx, ry, border);
        ASSERT_TRUE(row_score->require_normalize());
        auto select = make_shared<CaptureSelect>();
        DisparityBlockMatchRowFormat<F32, F32> alg(rx, ry, row_score, select);
        alg.configure(dmin, range);
        alg.set_border(border);
        alg.set_catastrophic_reset(reset);
        alg.process(left, right);
        ASSERT_EQ(height, select->rows.size());

        auto L = FactoryImageBorder::create_SB<F32>(border);
        auto R = FactoryImageBorder::create_SB<F32>(border);
        L->setImage(left);
        R->setImage(right);
        for (int32_t y = 0; y < (int32_t)height; y++) {
            for (int32_t d = dmin; d < dmin + range; d++) {
                for (int32_t x = 0; x < (int32_t)width; x++) {
                    if (x - d < 0 || x - d >= (int32_t)width)
                        continue;
                    RegionStats s = region_stats(*L, *R, x, y, d, rx, ry);
                    double expected;
                    if (ncc) {
                        double sigma_l = std::sqrt(std::max(0.0, s.mean_ll - s.mean_l*s.mean_l));
                        double sigma_r = std::sqrt(std::max(0.0, s.mean_rr - s.mean_r*s.mean_r));
                        expected = (s.mean_lr - s.mean_l*s.mean_r)/(eps + sigma_l*sigma_r);
                    } else {
                        double n = (2*rx + 1)*(2*ry + 1);
                        expected = s.ssd - n*(s.mean_l - s.mean_r)*(s.mean_l - s.mean_r);
                    }
                    F32 found = select->rows[y][width*(d - dmin) + x];
                    // NCC is from -1 to 1. ZSSD's error is relative to the SSD it's computed from
                    double tol = ncc ? 1e-5 : 1e-5*std::max(1.0, s.ssd);
                    ASSERT_NEAR(expected, found, tol) << (ncc ? "NCC " : "ZSSD ") << width << "x" << height
                            << " r=" << rx << "," << ry << " border=" << border << " at " << x << "," << y
                            << " d=" << d;
                }
            }
        }
    }
}

TEST(BlockRowScoreZssd, block_match_compare_to_naive) {
    for (BorderType border : {REFLECT, EXTENDED, WRAP, ZERO}) {
        normalized_compare_to_naive<BlockRowScoreZssd>(false, 30, 25, 2, 2, -3, 10, border, 25);
        normalized_compare_to_naive<BlockRowScoreZssd>(false, 31, 23, 3, 1, 0, 40, border, 4);
    }
    // smaller than the region
    normalized_compare_to_naive<BlockRowScoreZssd>(false, 3, 2, 2, 3, -1, 3, REFLECT, 25);
}

TEST(BlockRowScoreNcc, block_match_compare_to_naive) {
    for (BorderType border : {REFLECT, EXTENDED, WRAP, ZERO}) {
        normalized_compare_to_naive<BlockRowScoreNcc>(true, 30, 25, 2, 2, -3, 10, border, 25);
        normalized_compare_to_naive<BlockRowScoreNcc>(true, 31, 23, 3, 1, 0, 40, border, 4);
    }
    normalized_compare_to_naive<BlockRowScoreNcc>(true, 3, 2, 2, 3, -1, 3, REFLECT, 25);
}

namespace {
    /// With integer valued images the floating point scores are exact, so F32 images must select the same
    /// disparity and score as U8 images
    template<class D, class RowScoreU8, class RowScoreF32>
    void f32_same_as_u8( const Config& c, bool squared ) {
        Gray<U8> left(1, 1), right(1, 1);
        create_stereo_pair(left, right, c.width, c.height, 4);
        Gray<F32> left_f(c.width, c.height), right_f(c.width, c.height);
        for (uint32_t i = 0; i < left.data_length; i++) {
            left_f.data[i] = left.data[i];
            right_f.data[i] = right.data[i];
        }
        // The integer texture threshold is discretized, so use one that's exact as a float
        const double texture = c.texture > 0 ? 0.25 : 0;

        auto select_u8 = make_shared<SelectErrorWithChecks_S32<D>>(c.max_error, c.r2l, texture, squared);
        DisparityBlockMatchRowFormat<U8, int32_t> alg_u8(c.rx, c.ry, make_shared<RowScoreU8>(), select_u8);
        auto select_f32 = make_shared<SelectErrorWithChecks_F32<D>>(c.max_error, c.r2l, texture, squared);
        DisparityBlockMatchRowFormat<F32, F32> alg_f32(c.rx, c.ry, make_shared<RowScoreF32>(), select_f32);
        alg_u8.configure(c.dmin, c.range);
        alg_u8.set_border(c.border);
        alg_f32.configure(c.dmin, c.range);
        alg_f32.set_border(c.border);
        alg_f32.set_catastrophic_reset(3);

        Gray<D> disparity_u8(c.width, c.height), disparity_f32(c.width, c.height);
        Gray<F32> score_u8(c.width, c.height), score_f32(c.width, c.height);
        ImageMiscOps::fill(score_u8, -2.0f);
        ImageMiscOps::fill(score_f32, -2.0f);
        select_u8->set_output(disparity_u8, &score_u8);
        select_f32->set_output(disparity_f32, &score_f32);
        alg_u8.process(left, right);
        alg_f32.process(left_f, right_f);
        check_equals(disparity_u8, disparity_f32, (D)0);
        check_equals(score_u8, score_f32, 0.0f);
    }
}

TEST(DisparityBlockMatchRowFormat, F32_same_as_U8) {
    for (const auto& c : CONFIGS) {
        f32_same_as_u8<U8, BlockRowScoreSad<U8>, BlockRowScoreSad<F32>>(c, false);
        f32_same_as_u8<F32, BlockRowScoreSad<U8>, BlockRowScoreSad<F32>>(c, false);
        // SSD with the max error scaled to its units
        Config ssd = c;
        ssd.max_error = c.max_error*c.max_error;
        f32_same_as_u8<U8, BlockRowScoreSsd<U8>, BlockRowScoreSsd<F32>>(ssd, true);
        f32_same_as_u8<F32, BlockRowScoreSsd<U8>, BlockRowScoreSsd<F32>>(ssd, true);
    }
}

TEST(DisparityBlockMatchRowFormat, catastrophic_reset_must_be_positive) {
    auto select = make_shared<SelectErrorWithChecks_F32<U8>>(0, -1, 0);
    DisparityBlockMatchRowFormat<F32, F32> alg(2, 2, make_shared<BlockRowScoreSad<F32>>(), select);
    EXPECT_THROW(alg.set_catastrophic_reset(0), std::invalid_argument);
    EXPECT_NO_THROW(alg.set_catastrophic_reset(1));
}

namespace {
    /// Brute force correlation selection of one pixel from a score volume. Same rules as BoofCV's
    /// SelectCorrelationWithChecks_F32: largest score wins, the texture check is relative to the worst score
    struct NaiveCorrelation {
        int32_t width, dmin, dmax, r2l;
        float texture;
        // volume[x*range + d - dmin]
        vector<float> volume;

        int32_t range() const { return dmax - dmin + 1; }

        float at( int32_t x, int32_t d ) const { return volume[x*range() + d - dmin]; }

        /// @return disparity relative to dmin, or range() if invalid
        int32_t select( int32_t x, float& score, float& subpixel ) const {
            const int32_t invalid = range();
            const int32_t d0 = std::max(dmin, x - (width - 1));
            const int32_t d1 = std::min(x, dmax);
            if (d0 > d1) {
                score = std::numeric_limits<float>::quiet_NaN();
                subpixel = (float)invalid;
                return invalid;
            }
            int32_t best = d0;
            float worst = at(x, d0);
            for (int32_t d = d0; d <= d1; d++) {
                if (at(x, d) > at(x, best))
                    best = d;
                worst = std::min(worst, at(x, d));
            }
            score = at(x, best);
            int32_t result = best - dmin;

            if (r2l >= 0) {
                int32_t xr = x - best;
                int32_t best_r = 0;
                bool found = false;
                for (int32_t d = dmin; d <= dmax; d++) {
                    if (xr + d < 0 || xr + d >= width)
                        continue;
                    if (!found || at(xr + d, d) > at(xr + best_r, best_r))
                        best_r = d;
                    found = true;
                }
                if (std::abs(best_r - best) > r2l)
                    result = invalid;
            }

            if (texture > 0 && result != invalid && d1 - d0 + 1 >= 3) {
                float second = worst;
                for (int32_t d = d0; d <= d1; d++) {
                    if (std::abs(d - best) > 1)
                        second = std::max(second, at(x, d));
                }
                score -= worst;
                second -= worst;
                if (score - second <= texture*second)
                    result = invalid;
            }

            if (result == invalid || best - 1 < d0 || best + 1 > d1) {
                subpixel = (float)result;
            } else {
                float c0 = at(x, best - 1), c1 = at(x, best), c2 = at(x, best + 1);
                subpixel = (float)result + (c0 - c2)/(2.0f*(c0 - 2.0f*c1 + c2));
            }
            return result;
        }
    };

    template<class D>
    void correlation_compare_to_naive( int32_t width, int32_t dmin, int32_t range, int32_t r2l, float texture ) {
        NaiveCorrelation naive{width, dmin, dmin + range - 1, r2l, texture, {}};
        std::mt19937 gen(0xBEEF + width + range);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        naive.volume.resize(width*range);
        // scores in the selector's layout, scores[width*(d - dmin) + x]
        vector<float> scores(width*range);
        for (int32_t x = 0; x < width; x++) {
            for (int32_t d = dmin; d < dmin + range; d++) {
                float v = dist(gen);
                naive.volume[x*range + d - dmin] = v;
                scores[width*(d - dmin) + x] = v;
            }
        }

        SelectCorrelationWithChecks_F32<D> select(r2l, texture);
        Gray<D> disparity(width, 3);
        Gray<F32> score(width, 3);
        ImageMiscOps::fill(score, -2.0f);
        select.set_output(disparity, &score);
        select.configure(dmin, dmin + range - 1, width);
        select.process(1, scores.data());

        uint32_t valid = 0;
        for (int32_t x = 0; x < width; x++) {
            float expected_score, subpixel;
            int32_t expected = naive.select(x, expected_score, subpixel);
            if constexpr (std::is_same<D, F32>::value)
                ASSERT_EQ(subpixel, disparity.at(x, 1)) << x << " dmin=" << dmin << " range=" << range;
            else
                ASSERT_EQ(expected, (int32_t)disparity.at(x, 1)) << x << " dmin=" << dmin << " range=" << range;
            if (std::isnan(expected_score))
                ASSERT_TRUE(std::isnan(score.at(x, 1))) << x;
            else
                ASSERT_EQ(expected_score, score.at(x, 1)) << x;
            if (expected != range)
                valid++;
        }
        ASSERT_GT(valid, 0u);
    }
}

TEST(SelectCorrelationWithChecks_F32, compare_to_naive) {
    // width, dmin, range, r2l, texture
    for (auto c : {std::tuple{30, 0, 10, -1, 0.0f}, {30, -4, 12, -1, 0.0f}, {30, 3, 8, 1, 0.0f},
                   {30, -5, 12, 0, 0.0f}, {30, 0, 10, -1, 0.3f}, {12, 0, 20, 1, 0.1f}, {31, -3, 15, 1, 0.15f}}) {
        auto [width, dmin, range, r2l, texture] = c;
        correlation_compare_to_naive<U8>(width, dmin, range, r2l, texture);
        correlation_compare_to_naive<F32>(width, dmin, range, r2l, texture);
    }
}

TEST(SelectCorrelationWithChecks_F32, range_too_large_for_U8) {
    SelectCorrelationWithChecks_F32<U8> select(-1, 0);
    EXPECT_THROW(select.configure(0, 254, 30), std::invalid_argument);
    EXPECT_NO_THROW(select.configure(0, 253, 30));
}
