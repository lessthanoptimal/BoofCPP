#ifndef BOOFCPP_THRESHOLD_CHECKS_H
#define BOOFCPP_THRESHOLD_CHECKS_H

/// Checks of the threshold filters in tested::, shared by every variant. Include a variant header, e.g.
/// variant_strict.h, first. Each variant's tests call the checks for the image types it supports

#include "boofcv/config_types.h"
#include "boofcv/image_misc_ops.h"
#include "testing_utils.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <random>

namespace threshold_checks {
    using namespace boofcv;

    template<class E>
    using Filter = std::unique_ptr<InputToBinary<Gray<E>>>;

    /// Creates a block filter with the requested block width, scale and direction
    template<class E>
    using BlockFactory = std::function<Filter<E>( uint32_t block_width, double scale, bool down )>;

    template<class E>
    BlockFactory<E> block_mean( bool local_blocks = true ) {
        return [=]( uint32_t width, double scale, bool down ) -> Filter<E> {
            return std::make_unique<tested::ThresholdBlockMean<E>>(ConfigLength::fixed(width), local_blocks, scale,
                                                                   down);
        };
    }

    template<class E>
    BlockFactory<E> block_otsu( bool local_blocks = true ) {
        return [=]( uint32_t width, double scale, bool down ) -> Filter<E> {
            return std::make_unique<tested::ThresholdBlockOtsu<E>>(false, ConfigLength::fixed(width), 0.0, scale,
                                                                   down, local_blocks);
        };
    }

    template<class E>
    BlockFactory<E> block_min_max( bool local_blocks = true ) {
        return [=]( uint32_t width, double scale, bool down ) -> Filter<E> {
            return std::make_unique<tested::ThresholdBlockMinMax<E>>(1.0f, ConfigLength::fixed(width), local_blocks,
                                                                     (float)scale, down);
        };
    }

    /// Thresholding down and up give inverse images when no block is textureless
    template<class E>
    void block_toggle_down( const BlockFactory<E>& factory ) {
        std::mt19937 gen(0xBEEF);
        Gray<E> input(100, 120);
        ImageMiscOps::fill_uniform(input, (E)0, (E)255, gen);

        Gray<U8> down(input.width, input.height);
        Gray<U8> up(input.width, input.height);
        factory(6, 1.0, true)->process(input, down);
        factory(6, 1.0, false)->process(input, up);

        for (uint32_t y = 0; y < down.height; y++) {
            for (uint32_t x = 0; x < down.width; x++) {
                ASSERT_TRUE(down.at(x, y) == !up.at(x, y));
            }
        }
    }

    /// A block wider than the image is an error
    template<class E>
    void block_width_larger_than_image( const BlockFactory<E>& factory ) {
        Gray<E> input(10, 12);
        Gray<U8> output(input.width, input.height);
        EXPECT_ANY_THROW(factory(20, 1.0, true)->process(input, output));
    }

    /// A sub-image gives the same result as the whole image
    template<class E>
    void block_subimage( const BlockFactory<E>& factory ) {
        std::mt19937 gen(0xBEEF);
        Gray<E> input(100, 120);
        ImageMiscOps::fill_uniform(input, (E)0, (E)255, gen);
        Gray<U8> expected(input.width, input.height);

        Gray<E> sub_input = create_subimage(input);
        Gray<U8> sub_output = create_subimage(expected);

        auto alg = factory(14, 1.0, true);
        alg->process(input, expected);
        alg->process(sub_input, sub_output);
        check_equals(expected, sub_output);

        // so that it frees the memory and valgrind doesn't complain
        sub_input.subimage = false;
        sub_output.subimage = false;
    }

    template<class E>
    void block_all( const BlockFactory<E>& factory ) {
        block_toggle_down(factory);
        block_width_larger_than_image(factory);
        block_subimage(factory);
    }

    /// Scale adjusts the Otsu threshold the same way as BoofCV's GlobalBinaryFilter
    inline void global_otsu_scale() {
        Gray<U8> img(20, 10);
        for (uint32_t i = 0; i < img.total_pixels(); i++) img.data[i] = static_cast<U8>((i*37)%256);
        Gray<U8> found(20, 10);
        Gray<U8> expected(20, 10);

        double otsu = tested::ThresholdOps::computeOtsu(img, (U8)0, (U8)255, false); // min=0 so no offset

        for (double scale : {1.0, 0.0, 0.5, 1.2, 3.0}) {
            for (bool down : {true, false}) {
                tested::GlobalOtsuBinaryFilter<U8> alg(0, 255, down, scale);
                alg.process(img, found);

                double s = down ? scale : (scale <= 0.0 ? 0.0 : 1.0/scale);
                auto threshold = static_cast<int32_t>(otsu*s);
                for (uint32_t i = 0; i < img.total_pixels(); i++) {
                    int v = img.data[i];
                    expected.data[i] = static_cast<U8>(down ? v <= threshold : v > threshold);
                }
                for (uint32_t i = 0; i < img.total_pixels(); i++)
                    ASSERT_EQ(expected.data[i], found.data[i]) << "scale=" << scale << " down=" << down;
            }
        }
    }

    /// The Otsu threshold is a histogram index offset by min_value, same as Java
    inline void global_otsu_min_value_offset() {
        Gray<U8> img(20, 10);
        for (uint32_t i = 0; i < img.total_pixels(); i++) img.data[i] = static_cast<U8>(20 + (i*37)%181);
        Gray<U8> found(20, 10);

        int threshold = static_cast<int>(tested::ThresholdOps::computeOtsu(img, (U8)20, (U8)200, false)) + 20;
        tested::GlobalOtsuBinaryFilter<U8> alg(20, 200, true);
        alg.process(img, found);
        for (uint32_t i = 0; i < img.total_pixels(); i++)
            ASSERT_EQ(img.data[i] <= threshold, found.data[i]);
    }

    /// Thresholds are doubles like in Java. Out of range values must not wrap and fractions truncate like (int)
    inline void global_fixed_double_threshold() {
        Gray<U8> img(7, 5);
        for (uint32_t i = 0; i < img.total_pixels(); i++) img.data[i] = static_cast<U8>((i*41)%256);
        Gray<U8> found(7, 5);

        for (double t : {-1.0, 0.0, 125.0, 125.5, 255.0, 300.0}) {
            for (bool down : {true, false}) {
                tested::GlobalFixedBinaryFilter<U8> alg(t, down);
                alg.process(img, found);
                auto ti = static_cast<int>(t);
                for (uint32_t i = 0; i < img.total_pixels(); i++) {
                    int v = img.data[i];
                    ASSERT_EQ(down ? v <= ti : v > ti, found.data[i]) << "t=" << t << " down=" << down;
                }
            }
        }
    }

    /// Local mean threshold computed pixel by pixel: the mean along x of the pixels inside the image, rounded, then
    /// the mean of those along y, rounded. Same as BoofCV's BlurImageOps.mean() for U8
    inline Gray<U8> naive_local_mean( const Gray<U8>& input, const ConfigLength& region, float scale, bool down ) {
        const auto radius = (int32_t)region.computeI(std::min(input.width, input.height))/2;
        const auto width = (int32_t)input.width, height = (int32_t)input.height;
        Gray<U8> horizontal(input.width, input.height), output(input.width, input.height);
        for (int32_t y = 0; y < height; y++) {
            for (int32_t x = 0; x < width; x++) {
                int32_t sum = 0, count = 0;
                for (int32_t i = std::max(0, x - radius); i <= std::min(width - 1, x + radius); i++, count++)
                    sum += input.at(i, y);
                horizontal.at(x, y) = (U8)((sum + count/2)/count);
            }
        }
        for (int32_t y = 0; y < height; y++) {
            for (int32_t x = 0; x < width; x++) {
                int32_t sum = 0, count = 0;
                for (int32_t i = std::max(0, y - radius); i <= std::min(height - 1, y + radius); i++, count++)
                    sum += horizontal.at(x, i);
                const auto mean = (U8)((sum + count/2)/count);
                const U8 v = input.at(x, y);
                output.at(x, y) = (U8)(down ? v <= mean*scale : v*scale > mean);
            }
        }
        return output;
    }

    /// Compares with naive_local_mean() on random, flat and smooth images, where many pixels equal their mean, with
    /// regions from 2 pixels to wider than the image, a region relative to the image size, and sub-images
    inline void local_mean_naive() {
        std::mt19937 gen(0xBEEF);
        const ConfigLength regions[] = {ConfigLength::fixed(2), ConfigLength::fixed(3), ConfigLength::fixed(8),
                                        ConfigLength::fixed(41), ConfigLength::fixed(1000),
                                        ConfigLength::relative(0.2, 3)};
        for (auto shape : {std::pair<uint32_t, uint32_t>{2, 2}, {5, 3}, {37, 29}, {64, 64}, {161, 47}}) {
            for (int content = 0; content < 3; content++) {
                Gray<U8> input(shape.first, shape.second);
                ImageMiscOps::fill_uniform(input, (U8)0, (U8)255, gen);
                if (content == 1) {
                    ImageMiscOps::fill(input, (U8)128);
                } else if (content == 2) {
                    for (uint32_t y = 0; y < input.height; y++)
                        for (uint32_t x = 0; x < input.width; x++)
                            input.at(x, y) = (U8)((x*3 + y*2)%256);
                }
                for (const ConfigLength& region : regions) {
                    for (bool down : {true, false}) {
                        for (float scale : {0.95f, 1.0f, 1.05f}) {
                            Gray<U8> expected = naive_local_mean(input, region, scale, down);
                            tested::LocalMeanBinaryFilter<U8> alg(region, scale, down);
                            Gray<U8> found(input.width, input.height);
                            alg.process(input, found);
                            check_equals(expected, found);

                            Gray<U8> sub_input = create_subimage(input);
                            Gray<U8> sub_found = create_subimage(found);
                            alg.process(sub_input, sub_found);
                            check_equals(expected, sub_found);
                            sub_input.subimage = false;
                            sub_found.subimage = false;
                        }
                    }
                }
            }
        }
    }

    /// A region narrower than 2 pixels has radius 0, which is an error
    template<class E>
    void local_mean_zero_radius() {
        Gray<E> input(10, 12);
        Gray<U8> output(input.width, input.height);
        tested::LocalMeanBinaryFilter<E> alg(ConfigLength::fixed(1), 1.0f, true);
        EXPECT_ANY_THROW(alg.process(input, output));
    }
}

#endif
