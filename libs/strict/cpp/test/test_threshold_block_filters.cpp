#include "gtest/gtest.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>
#include "boofcv/threshold_block_filters.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"
#include "variant_strict.h"
#include "shared/threshold_checks.h"

using namespace std;
using namespace boofcv;

class DummyThresholdBlockCommon : public ThresholdBlockCommon<U8> {
public:
    explicit DummyThresholdBlockCommon( uint32_t width )
            : ThresholdBlockCommon(ConfigLength::fixed(width), true) {}

protected:
    void begin( uint32_t width ) override {}
    void statistics( const Gray<U8>& input, uint32_t by ) override {}
    void threshold_rows( const Gray<U8>& input, Gray<U8>& output, uint32_t by ) override {}
};

class ThresholdBlockCommonTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockCommonTest, selectBlockSize) {
    uint32_t width = 30;

    DummyThresholdBlockCommon alg(width);

    alg.selectBlockSize(300,330,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(329,301,width);
    ASSERT_EQ(32,alg.blockWidth);
    ASSERT_EQ(30,alg.blockHeight);

    alg.selectBlockSize(301,329,width);
    ASSERT_EQ(30,alg.blockWidth);
    ASSERT_EQ(32,alg.blockHeight);
}

class ThresholdBlockMeanTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMeanTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_mean<U8>()); }
TEST_F(ThresholdBlockMeanTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_mean<F32>()); }

class ThresholdBlockOtsuTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockOtsuTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_otsu<U8>()); }
TEST_F(ThresholdBlockOtsuTest, standard_tests_F32) { threshold_checks::block_all(threshold_checks::block_otsu<F32>()); }

class ThresholdBlockMinMaxTest : public boofcv::BoofCPPTest {};

TEST_F(ThresholdBlockMinMaxTest, standard_tests_U8) { threshold_checks::block_all(threshold_checks::block_min_max<U8>()); }
TEST_F(ThresholdBlockMinMaxTest, standard_tests_F32) {
    threshold_checks::block_all(threshold_checks::block_min_max<F32>());
}

/// The block filters computed one block at a time, in the order of BoofCV's Java code: each block's statistics, then
/// each block's threshold from the statistics of its local region
namespace reference {
    struct Blocks {
        uint32_t width, height;  // size of a block, except the last in each row and column
        uint32_t cols, rows;     // number of blocks
        bool local;

        Blocks( const ConfigLength& requested, bool local, uint32_t image_width, uint32_t image_height )
                : local(local) {
            auto r = (uint32_t)requested.computeI(std::min(image_width, image_height));
            width = image_width/(image_width/r);
            height = image_height/(image_height/r);
            cols = image_width/width;
            rows = image_height/height;
        }

        /// Calls stat(index, x0, y0, x1, y1) for each block, then threshold(index, x0, y0, x1, y1, region) where
        /// region lists the indexes of the blocks in its local region
        template<class Stat, class Threshold>
        void process( uint32_t image_width, uint32_t image_height, Stat stat, Threshold threshold ) const {
            auto x1 = [&]( uint32_t bx ) { return bx == cols - 1 ? image_width : (bx + 1)*width; };
            auto y1 = [&]( uint32_t by ) { return by == rows - 1 ? image_height : (by + 1)*height; };
            for (uint32_t by = 0; by < rows; by++)
                for (uint32_t bx = 0; bx < cols; bx++)
                    stat(by*cols + bx, bx*width, by*height, x1(bx), y1(by));
            for (uint32_t by = 0; by < rows; by++) {
                for (uint32_t bx = 0; bx < cols; bx++) {
                    std::vector<uint32_t> region;
                    const uint32_t r = local ? 1 : 0;
                    for (uint32_t y = by > r ? by - r : 0; y <= std::min(rows - 1, by + r); y++)
                        for (uint32_t x = bx > r ? bx - r : 0; x <= std::min(cols - 1, bx + r); x++)
                            region.push_back(y*cols + x);
                    threshold(bx*width, by*height, x1(bx), y1(by), region);
                }
            }
        }
    };

    template<class E>
    Gray<U8> mean( const Gray<E>& input, const ConfigLength& requested, bool local, double scale_d, bool down ) {
        typedef typename TypeInfo<E>::sum_type sum_type;
        const auto scale = (float)scale_d;
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<E> means(blocks.cols*blocks.rows);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            sum_type sum = 0;
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    sum += input.at(x, y);
            const uint32_t area = (x1 - x0)*(y1 - y0);
            if (std::is_integral_v<E>)
                sum = static_cast<sum_type>(scale*sum/area + 0.5f);
            else
                sum = static_cast<sum_type>(scale*sum/area);
            means[index] = static_cast<E>(sum);
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            sum_type sum = 0;
            for (uint32_t i : region)
                sum += means[i];
            const E threshold = sum/(uint32_t)region.size();
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    output.at(x, y) = (U8)(down == (input.at(x, y) <= threshold));
        });
        return output;
    }

    template<class E>
    Gray<U8> otsu( const Gray<E>& input, bool otsu2, const ConfigLength& requested, double tuning, double scale,
                   bool down, bool local ) {
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<uint32_t> histograms(blocks.cols*blocks.rows*256, 0);
        ComputeOtsu otsu(otsu2, tuning, down, scale);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    histograms[index*256 + (U8)input.at(x, y)]++;
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            GrowArray<uint32_t> histogram(256);
            histogram.fill(0);
            uint32_t total = 0;
            for (uint32_t i : region) {
                for (uint32_t v = 0; v < 256; v++) {
                    histogram[v] += histograms[i*256 + v];
                    total += histograms[i*256 + v];
                }
            }
            otsu.compute(histogram, total);
            for (uint32_t y = y0; y < y1; y++)
                for (uint32_t x = x0; x < x1; x++)
                    output.at(x, y) = (U8)(down == ((U8)input.at(x, y) <= otsu.threshold));
        });
        return output;
    }

    /// The textureless spread as BoofCV computes it for integer images: -1 if negative, otherwise Math.round saturated
    /// to an int, with NaN as 0
    template<class E>
    auto spread( double minimumSpread ) {
        typedef typename TypeInfo<E>::sum_type sum_type;
        if constexpr (std::is_integral_v<E>) {
            if (minimumSpread < 0)
                return (int64_t)-1;
            if (std::isnan(minimumSpread))
                return (int64_t)0;
            // Math.round. x + 0.5 is exact in x86's long double for the doubles tested, so floor rounds ties up
            const long double v = std::floor((long double)minimumSpread + 0.5L);
            return (int64_t)std::min(v, (long double)std::numeric_limits<int32_t>::max());
        } else {
            return (sum_type)minimumSpread;
        }
    }

    template<class E>
    Gray<U8> min_max( const Gray<E>& input, double minimumSpread, const ConfigLength& requested, bool local,
                      float scale, bool down ) {
        typedef typename TypeInfo<E>::sum_type sum_type;
        Gray<U8> output(input.width, input.height);
        Blocks blocks(requested, local, input.width, input.height);
        std::vector<E> mins(blocks.cols*blocks.rows), maxs(blocks.cols*blocks.rows);
        blocks.process(input.width, input.height, [&]( uint32_t index, uint32_t x0, uint32_t y0, uint32_t x1,
                                                       uint32_t y1 ) {
            E min = input.at(x0, y0), max = min;
            for (uint32_t y = y0; y < y1; y++) {
                for (uint32_t x = x0; x < x1; x++) {
                    const E value = input.at(x, y);
                    if (value < min)
                        min = value;
                    else if (value > max)
                        max = value;
                }
            }
            mins[index] = min;
            maxs[index] = max;
        }, [&]( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const std::vector<uint32_t>& region ) {
            E min = std::numeric_limits<E>::max();
            E max = std::numeric_limits<E>::lowest();
            for (uint32_t i : region) {
                if (mins[i] < min)
                    min = mins[i];
                if (maxs[i] > max)
                    max = maxs[i];
            }
            const auto textureThreshold = spread<E>(minimumSpread);
            for (uint32_t y = y0; y < y1; y++) {
                for (uint32_t x = x0; x < x1; x++) {
                    if ((decltype(textureThreshold))(max - min) <= textureThreshold) {
                        output.at(x, y) = 1;
                    } else {
                        const auto average = static_cast<sum_type>(scale*((max + min)/2));
                        output.at(x, y) = (U8)(down == (input.at(x, y) <= average));
                    }
                }
            }
        });
        return output;
    }
}

namespace {
    enum class Content { UNIFORM, FLAT, SMOOTH, HARD };

    /// Test images. HARD has values which are hard for floating point: large and tiny magnitudes, negative values and
    /// signed zeros for F32 (when 'limited' is false), and long runs of equal values
    template<class E>
    Gray<E> create_image( uint32_t width, uint32_t height, Content content, bool limited, std::mt19937& gen ) {
        Gray<E> image(width, height);
        std::uniform_real_distribution<double> uniform(0, 255);
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                double v = 0;
                switch (content) {
                    case Content::UNIFORM: v = uniform(gen); break;
                    case Content::FLAT: v = 128; break;
                    case Content::SMOOTH: v = 100 + 60*std::sin(x*0.05)*std::cos(y*0.07) + uniform(gen)/50; break;
                    case Content::HARD: {
                        const uint32_t kind = (x/7 + y/5)%5;
                        if (kind == 0) v = 200;
                        else if (kind == 1) v = uniform(gen);
                        else if (std::is_integral_v<E> || limited) v = kind*50;
                        else if (kind == 2) v = 1e6 + uniform(gen);
                        else if (kind == 3) v = -uniform(gen)*1e-30;
                        else v = (x + y)%2 == 0 ? 0.0 : -0.0;
                        break;
                    }
                }
                image.at(x, y) = std::is_integral_v<E> ? (E)std::lround(v) : (E)v;
            }
        }
        return image;
    }

    const std::vector<std::pair<uint32_t, uint32_t>> SHAPES = {{16, 16}, {37, 29}, {100, 120}, {160, 47}, {401, 297}};
    const Content CONTENTS[] = {Content::UNIFORM, Content::FLAT, Content::SMOOTH, Content::HARD};

    /// The filter gives the expected image, for the whole image and a sub-image
    template<class E>
    void check_same( const Gray<U8>& expected, InputToBinary<Gray<E>>& alg, const Gray<E>& input ) {
        Gray<U8> found(input.width, input.height);
        alg.process(input, found);
        check_equals(expected, found);

        Gray<E> sub_input = create_subimage(input);
        Gray<U8> sub_found = create_subimage(found);
        alg.process(sub_input, sub_found);
        check_equals(expected, sub_found);
        sub_input.subimage = false;
        sub_found.subimage = false;
    }

    /// Calls check(input, width) for every test image and block width that fits it
    template<class E, class Check>
    void for_each_input( bool limited, Check check ) {
        std::mt19937 gen(0xBEEF);
        for (auto shape : SHAPES) {
            for (Content content : CONTENTS) {
                Gray<E> input = create_image<E>(shape.first, shape.second, content, limited, gen);
                for (uint32_t width : {2u, 5u, 14u, 40u}) {
                    if (width > std::min(shape.first, shape.second))
                        continue;
                    check(input, ConfigLength::fixed(width));
                }
            }
        }
    }

    template<class E>
    void block_mean_same_as_reference() {
        for_each_input<E>(false, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (bool down : {true, false})
                    for (double scale : {0.95, 1.0, 1.3}) {
                        ThresholdBlockMean<E> alg(width, local, scale, down);
                        check_same<E>(reference::mean(input, width, local, scale, down), alg, input);
                    }
        });
    }

    template<class E>
    void block_otsu_same_as_reference() {
        // F32 pixels must fit in U8, which they're cast to
        for_each_input<E>(true, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (bool down : {true, false})
                    for (bool otsu2 : {true, false})
                        for (auto tuning_scale : {std::pair<double, double>{0.0, 0.95}, {0.3, 1.3}}) {
                            const double tuning = tuning_scale.first, scale = tuning_scale.second;
                            ThresholdBlockOtsu<E> alg(otsu2, width, tuning, scale, down, local);
                            check_same<E>(reference::otsu(input, otsu2, width, tuning, scale, down, local), alg,
                                          input);
                        }
        });
    }

    template<class E>
    void block_min_max_same_as_reference() {
        for_each_input<E>(false, [&]( const Gray<E>& input, const ConfigLength& width ) {
            for (bool local : {true, false})
                for (bool down : {true, false})
                    for (float spread : {0.0f, 1.0f, 10.0f})
                        for (float scale : {0.95f, 1.0f, 1.3f}) {
                            ThresholdBlockMinMax<E> alg(spread, width, local, scale, down);
                            check_same<E>(reference::min_max(input, spread, width, local, scale, down), alg, input);
                        }
        });
    }

    /// F32 image with NaN pixels, including the first pixel of some blocks, which is never replaced
    Gray<F32> nan_image() {
        std::mt19937 gen(0xBEEF);
        Gray<F32> input = create_image<F32>(100, 120, Content::UNIFORM, true, gen);
        for (uint32_t y = 0; y < input.height; y += 3)
            for (uint32_t x = 0; x < input.width; x += 7)
                input.at(x, y) = std::numeric_limits<F32>::quiet_NaN();
        return input;
    }
}

TEST_F(ThresholdBlockMeanTest, same_as_reference_U8) { block_mean_same_as_reference<U8>(); }
TEST_F(ThresholdBlockMeanTest, same_as_reference_F32) { block_mean_same_as_reference<F32>(); }
TEST_F(ThresholdBlockOtsuTest, same_as_reference_U8) { block_otsu_same_as_reference<U8>(); }
TEST_F(ThresholdBlockOtsuTest, same_as_reference_F32) { block_otsu_same_as_reference<F32>(); }
TEST_F(ThresholdBlockMinMaxTest, same_as_reference_U8) { block_min_max_same_as_reference<U8>(); }
TEST_F(ThresholdBlockMinMaxTest, same_as_reference_F32) { block_min_max_same_as_reference<F32>(); }

TEST_F(ThresholdBlockMeanTest, nan_same_as_reference) {
    Gray<F32> input = nan_image();
    for (bool local : {true, false}) {
        ThresholdBlockMean<F32> alg(ConfigLength::fixed(10), local, 1.0, true);
        check_same<F32>(reference::mean(input, ConfigLength::fixed(10), local, 1.0, true), alg, input);
    }
}

TEST_F(ThresholdBlockMinMaxTest, nan_same_as_reference) {
    Gray<F32> input = nan_image();
    for (bool local : {true, false}) {
        for (bool down : {true, false}) {
            ThresholdBlockMinMax<F32> alg(1.0f, ConfigLength::fixed(7), local, 1.0f, down);
            check_same<F32>(reference::min_max(input, 1.0f, ConfigLength::fixed(7), local, 1.0f, down), alg, input);
        }
    }
}

/// A signed image whose scaled averages are below the lowest S16 value, and textureless blocks
TEST_F(ThresholdBlockMinMaxTest, average_outside_signed_range) {
    std::mt19937 gen(0xBEEF);
    Gray<S16> input(64, 48);
    ImageMiscOps::fill_uniform(input, (S16)-32768, (S16)-32000, gen);
    for (uint32_t y = 0; y < 16; y++)
        for (uint32_t x = 0; x < 16; x++)
            input.at(x, y) = 100;
    for (bool local : {true, false}) {
        for (bool down : {true, false}) {
            for (float scale : {0.95f, 1.3f}) {
                ThresholdBlockMinMax<S16> alg(1.0f, ConfigLength::fixed(8), local, scale, down);
                check_same<S16>(reference::min_max(input, 1.0f, ConfigLength::fixed(8), local, scale, down), alg,
                                input);
            }
        }
    }
}

/// An image with only negative values, whose region maximum is below zero
TEST_F(ThresholdBlockMinMaxTest, negative_pixels) {
    Gray<F32> input(12, 8);
    for (uint32_t y = 0; y < input.height; y++)
        for (uint32_t x = 0; x < input.width; x++)
            input.at(x, y) = (x + y)%2 == 0 ? -10.0f : -20.0f;
    for (bool down : {true, false}) {
        ThresholdBlockMinMax<F32> alg(1.0f, ConfigLength::fixed(4), true, 1.0f, down);
        Gray<U8> output(input.width, input.height);
        alg.process(input, output);
        // the threshold is (-10 + -20)/2 = -15
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                EXPECT_EQ(down == (input.at(x, y) == -20.0f), output.at(x, y) == 1) << x << " " << y;
        check_same<F32>(reference::min_max(input, 1.0f, ConfigLength::fixed(4), true, 1.0f, down), alg, input);
    }
}

namespace {
    /// Counts the 1s from thresholding up an image of alternating 'a' and 'b'
    uint32_t checkerboard_ones( U8 a, U8 b, double spread ) {
        Gray<U8> input(20, 16);
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                input.at(x, y) = (x + y)%2 == 0 ? a : b;
        ThresholdBlockMinMax<U8> alg(spread, ConfigLength::fixed(5), true, 1.0f, false);
        Gray<U8> output(input.width, input.height);
        alg.process(input, output);
        check_same<U8>(reference::min_max(input, spread, ConfigLength::fixed(5), true, 1.0f, false), alg, input);
        uint32_t ones = 0;
        for (uint32_t y = 0; y < input.height; y++)
            for (uint32_t x = 0; x < input.width; x++)
                ones += output.at(x, y);
        return ones;
    }
}

/// Any negative spread disables the textureless check. Thresholding up, a flat image is then all 0, not all 1
TEST_F(ThresholdBlockMinMaxTest, negative_spread_disables) {
    for (double spread : {-0.1, -1.0})
        EXPECT_EQ(0u, checkerboard_ones(128, 128, spread)) << spread;
    EXPECT_EQ(20u*16u, checkerboard_ones(128, 128, 0.0));
}

/// Integer images round the spread like Java's Math.round, ties up, then cast it to an int
TEST_F(ThresholdBlockMinMaxTest, texture_threshold) {
    auto threshold = []( double minimumSpread ) {
        return ThresholdBlockMinMax<U8>(minimumSpread, ConfigLength::fixed(5), true, 1.0, false).texture_threshold();
    };
    EXPECT_EQ(-1, threshold(-0.1));
    EXPECT_EQ(-1, threshold(-1e20));
    EXPECT_EQ(0, threshold(0));
    EXPECT_EQ(0, threshold(std::nextafter(0.5, 0.0)));
    EXPECT_EQ(1, threshold(0.5));
    EXPECT_EQ(2, threshold(2.4999999999));
    EXPECT_EQ(3, threshold(2.5));
    EXPECT_EQ(3, threshold(2.9999999999));
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), threshold(1e20));
    EXPECT_EQ(0, threshold(std::numeric_limits<double>::quiet_NaN()));
}

/// Every block filter rejects a block width of zero instead of dividing by it
TEST_F(ThresholdBlockMeanTest, zero_block_width) {
    Gray<U8> input(10, 12);
    Gray<U8> output(10, 12);
    ThresholdBlockMean<U8> alg(ConfigLength::fixed(0), true, 1.0, true);
    EXPECT_THROW(alg.process(input, output), std::invalid_argument);
}
