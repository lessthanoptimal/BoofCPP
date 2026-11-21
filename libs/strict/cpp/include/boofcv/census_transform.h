#ifndef BOOFCPP_CENSUS_TRANSFORM_H
#define BOOFCPP_CENSUS_TRANSFORM_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "boofcv/census_variants.h"
#include "boofcv/geometry_types.h"
#include "boofcv/image_types.h"
#include "boofcv/image_border.h"

namespace boofcv {

    /// Number of bits which are 1. Bit twiddling instead of a popcount instruction, which isn't part of baseline
    /// x86-64. Inside a loop the compiler can vectorize it.
    inline int32_t hamming( uint32_t a ) {
        a = a - ((a >> 1) & 0x55555555u);
        a = (a & 0x33333333u) + ((a >> 2) & 0x33333333u);
        a = (a + (a >> 4)) & 0x0F0F0F0Fu;
        a = a + (a >> 8);
        a = a + (a >> 16);
        return (int32_t)(a & 0x3Fu);
    }

    /// Number of bits which are 1 in a 64-bit value. Same as BoofCV's DescriptorDistance.hamming(long)
    inline int32_t hamming( uint64_t a ) {
        return hamming((uint32_t)a) + hamming((uint32_t)(a >> 32));
    }

    /// The Census Transform computes a bit mask for each pixel in the image. If a neighboring pixel is greater than
    /// the center pixel then that bit is set to 1. The error between two pixels is the hamming distance of their
    /// masks. Same as BoofCV's CensusTransform.
    ///
    /// Bits are assigned to the neighbors in row-major order, skipping the center pixel.
    ///
    /// Every transform, with bit k comparing sample k with the center:
    ///
    ///     inner image, for each row:
    ///         for each group of 8 samples:  a byte row of the group's bits, for every pixel of the row
    ///         out[x] = the group bytes combined, group g in bits 8g to 8g + 7  (one group: the byte row is out)
    ///     with a border:  each pixel within the largest sample distance of an edge, through the border
    ///
    /// The F32 5x5 inner image instead compares all 24 samples of a pixel in one pass, see fused_5x5().
    class CensusTransform {
    public:
        /// Census transform of the 3x3 region around each pixel. 8 bits.
        ///
        /// @param border How pixels outside the image are read. If null the image border isn't modified.
        template<class E>
        static void dense3x3( const Gray<E>& input, Gray<U8>& output, ImageBorder<E>* border ) {
            dense<1>(input, output, border);
        }

        /// Census transform of the 5x5 region around each pixel. 24 bits.
        ///
        /// @param border How pixels outside the image are read. If null the image border isn't modified.
        template<class E>
        static void dense5x5( const Gray<E>& input, Gray<S32>& output, ImageBorder<E>* border ) {
            dense<2>(input, output, border);
        }

        /// Census transform which samples the given points around each pixel. Bit i is set if the pixel at
        /// samples[i] relative to the center is greater than the center. At most 64 samples.
        ///
        /// Pixels which are within the largest sample distance (x or y) of the image edge are only written if there
        /// is a border. Same as BoofCV's CensusTransform.sample_S64()
        ///
        /// @param border How pixels outside the image are read. If null the image border isn't modified.
        template<class E>
        static void sample_S64( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples,
                                Gray<S64>& output, ImageBorder<E>* border ) {
            if (samples.size() > 64)
                throw std::invalid_argument("At most 64 samples");
            output.reshape(input.width, input.height);

            // The inner image is the same distance from each edge, the largest x or y of a sample
            int32_t radius = 0;
            for (const auto& p : samples) {
                radius = std::max(radius, std::abs(p.x));
                radius = std::max(radius, std::abs(p.y));
            }

            byte_rows(input, samples, radius, output);

            if (border != nullptr)
                sample_border(input, samples, radius, output, *border);
        }

        /// The census of sample_S64() for the pixels within 'radius' of an edge, read through 'border'. 'radius' is the
        /// largest x or y of a sample. Same as ImplCensusTransformBorder.sample_S64()
        template<class E>
        static void sample_border( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples, int32_t radius,
                                   Gray<S64>& output, ImageBorder<E>& border ) {
            const int32_t width = input.width;
            const int32_t height = input.height;
            border.setImage(input);

            for (int32_t y = 0; y < height; y++) {
                const bool whole_row = y < radius || y >= height - radius || width <= 2*radius;
                for (int32_t x = 0; x < width; x++) {
                    // skip the inner image
                    if (!whole_row && x == radius)
                        x = width - radius;
                    output.unsafe_at(x, y) = sample(border, x, y, samples);
                }
            }
        }

        /// The census of dense3x3() (R = 1) or dense5x5() (R = 2) for the pixels within R of an edge, read through
        /// 'border'. Same as ImplCensusTransformBorder.dense3x3_U8() and dense5x5_S32()
        template<int R, class E, class C>
        static void dense_border( const Gray<E>& input, Gray<C>& output, ImageBorder<E>& border ) {
            const int32_t width = input.width;
            const int32_t height = input.height;
            border.setImage(input);

            for (int32_t y = 0; y < height; y++) {
                const bool whole_row = y < R || y >= height - R || width <= 2*R;
                for (int32_t x = 0; x < width; x++) {
                    // skip the inner image
                    if (!whole_row && x == R)
                        x = width - R;
                    output.unsafe_at(x, y) = (C)region<R>(border, x, y);
                }
            }
        }

        /// Every point in a (2*radius_x+1) by (2*radius_y+1) block except the center, in row-major order.
        /// Same as BoofCV's CensusTransform.createBlockSamples()
        static std::vector<Point2D<int32_t>> create_block_samples( int32_t radius_x, int32_t radius_y ) {
            std::vector<Point2D<int32_t>> samples;
            samples.reserve((2*radius_x + 1)*(2*radius_y + 1) - 1);
            for (int32_t y = -radius_y; y <= radius_y; y++) {
                for (int32_t x = -radius_x; x <= radius_x; x++) {
                    if (x == 0 && y == 0)
                        continue;
                    samples.emplace_back(x, y);
                }
            }
            return samples;
        }

        /// 56 points inside a circle of radius 4. Same points in the same order as BoofCV's
        /// CensusTransform.createCircleSamples(), which goes down each column, i.e. the point's x is the outer loop.
        static std::vector<Point2D<int32_t>> create_circle_samples() {
            std::vector<Point2D<int32_t>> samples;
            for (int32_t row = 0; row < 9; row++) {
                const int32_t col0 = row <= 4 ? std::max(0, 3 - row) : row - 5;
                const int32_t col1 = 9 - col0;
                for (int32_t col = col0; col < col1; col++) {
                    if (row == 4 && col == 4)
                        continue;
                    samples.emplace_back(row - 4, col - 4);
                }
            }
            return samples;
        }

    private:
        /// Census of a single pixel from sample points, reading through the border.
        /// Same as ImplCensusTransformBorder.sample_S64()
        template<class E>
        static S64 sample( const ImageBorder<E>& border, int32_t cx, int32_t cy,
                           const std::vector<Point2D<int32_t>>& samples ) {
            const E center = border.get(cx, cy);
            uint64_t census = 0;
            uint64_t bit = 1;
            for (const auto& p : samples) {
                if (border.get(cx + p.x, cy + p.y) > center)
                    census |= bit;
                bit <<= 1;
            }
            return (S64)census;
        }

        template<int R, class E, class C>
        static void dense( const Gray<E>& input, Gray<C>& output, ImageBorder<E>* border ) {
            output.reshape(input.width, input.height);
            if constexpr (R == 2 && std::is_same<E, F32>::value)
                fused_5x5(input, output);
            else
                byte_rows(input, create_block_samples(R, R), R, output);

            if (border != nullptr)
                dense_border<R>(input, output, *border);
        }

        /// Array offsets of the samples from the center pixel
        template<class E>
        static std::vector<int32_t> sample_offsets( const Gray<E>& input,
                                                    const std::vector<Point2D<int32_t>>& samples ) {
            std::vector<int32_t> offsets(samples.size());
            for (size_t i = 0; i < samples.size(); i++)
                offsets[i] = samples[i].y*(int32_t)input.stride + samples[i].x;
            return offsets;
        }

        /// Bits of up to 8 samples into a byte per pixel: bits[x] = sum of (center[x + offsets[k]] > center[x]) << k.
        /// Comparing 8 samples in one pass over the row writes each byte once, instead of once per sample
        template<class E>
        static void byte_bits( const E* __restrict center, const int32_t* offsets, int32_t count, int32_t length,
                               uint8_t* __restrict bits ) {
            if (count == 8) {
                const int32_t o0 = offsets[0], o1 = offsets[1], o2 = offsets[2], o3 = offsets[3];
                const int32_t o4 = offsets[4], o5 = offsets[5], o6 = offsets[6], o7 = offsets[7];
                for (int32_t x = 0; x < length; x++) {
                    const E c = center[x];
                    bits[x] = (uint8_t)((center[x + o0] > c) | (center[x + o1] > c) << 1 | (center[x + o2] > c) << 2 |
                                        (center[x + o3] > c) << 3 | (center[x + o4] > c) << 4 |
                                        (center[x + o5] > c) << 5 | (center[x + o6] > c) << 6 |
                                        (center[x + o7] > c) << 7);
                }
            } else {
                for (int32_t x = 0; x < length; x++) {
                    uint8_t b = 0;
                    for (int32_t k = 0; k < count; k++)
                        b |= (uint8_t)((center[x + offsets[k]] > center[x]) << k);
                    bits[x] = b;
                }
            }
        }

        /// Inner image census, see CensusTransform. 'radius' is the largest x or y of a sample
        template<class E, class C>
        static void byte_rows( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples, int32_t radius,
                               Gray<C>& output ) {
            const int32_t height = input.height;
            const int32_t length = (int32_t)input.width - 2*radius;
            if (length <= 0 || height <= 2*radius)
                return;
            const std::vector<int32_t> offsets = sample_offsets(input, samples);
            const int32_t count = (int32_t)samples.size();
            const int32_t groups = (count + 7)/8;
            std::vector<uint8_t> rows((size_t)groups*length);

            for (int32_t y = radius; y < height - radius; y++) {
                const E* center = input.data + input.offset + y*input.stride + radius;
                C* __restrict out = output.data + output.offset + y*output.stride + radius;
                // a single group of a byte output is the output
                if constexpr (sizeof(C) == 1) {
                    byte_bits(center, offsets.data(), count, length, (uint8_t*)out);
                    continue;
                }
                for (int32_t g = 0; g < groups; g++)
                    byte_bits(center, offsets.data() + 8*g, std::min(8, count - 8*g), length,
                              rows.data() + (size_t)g*length);
                if (groups == 3) {
                    const uint8_t *b0 = rows.data(), *b1 = b0 + length, *b2 = b1 + length;
                    for (int32_t x = 0; x < length; x++)
                        out[x] = (C)((uint32_t)b0[x] | (uint32_t)b1[x] << 8 | (uint32_t)b2[x] << 16);
                } else {
                    for (int32_t x = 0; x < length; x++) {
                        uint64_t v = 0;
                        for (int32_t g = 0; g < groups; g++)
                            v |= (uint64_t)rows[(size_t)g*length + x] << (8*g);
                        out[x] = (C)v;
                    }
                }
            }
        }

        /// Inner image census of the 24 neighbors of a 5x5 region in one pass, each bit ORed into the output. Faster
        /// than byte rows for F32, whose comparisons are 32 bits wide like the output
        static void fused_5x5( const Gray<F32>& input, Gray<S32>& output ) {
            const int32_t height = input.height;
            const int32_t length = (int32_t)input.width - 4;
            if (length <= 0 || height <= 4)
                return;
            const std::vector<int32_t> offsets = sample_offsets(input, create_block_samples(2, 2));
            int32_t o[24];
            std::copy(offsets.begin(), offsets.end(), o);
            for (int32_t y = 2; y < height - 2; y++) {
                const F32* __restrict center = input.data + input.offset + y*input.stride + 2;
                S32* __restrict out = output.data + output.offset + y*output.stride + 2;
                for (int32_t x = 0; x < length; x++) {
                    const F32 c = center[x];
                    S32 v = 0;
#pragma GCC unroll 24
                    for (int32_t k = 0; k < 24; k++)
                        v |= (S32)(center[x + o[k]] > c) << k;
                    out[x] = v;
                }
            }
        }

        /// Census of a single pixel, reading through the border. Same as ImplCensusTransformBorder.regionNxN()
        template<int R, class E>
        static int32_t region( const ImageBorder<E>& border, int32_t cx, int32_t cy ) {
            const E center = border.get(cx, cy);
            int32_t census = 0;
            int32_t bit = 1;
            for (int32_t row = -R; row <= R; row++) {
                for (int32_t col = -R; col <= R; col++) {
                    if (row == 0 && col == 0)
                        continue;
                    if (border.get(cx + col, cy + row) > center)
                        census |= bit;
                    bit <<= 1;
                }
            }
            return census;
        }
    };

    /// Census transform as a filter from an input image to a census image. Same as BoofCV's FilterCensusTransform.
    ///
    /// @tparam E Input image type
    /// @tparam C Census image type
    template<class E, class C>
    class FilterCensusTransform {
    public:
        /// @param border How pixels outside the image are read. If null the image border isn't modified.
        explicit FilterCensusTransform( std::shared_ptr<ImageBorder<E>> border ) : border(std::move(border)) {}

        virtual ~FilterCensusTransform() = default;

        virtual void process( const Gray<E>& input, Gray<C>& output ) = 0;

    protected:
        std::shared_ptr<ImageBorder<E>> border;
    };

    /// 3x3 census transform. Same as BoofCV's FilterCensusTransformD33U8
    template<class E>
    class FilterCensusTransformD33U8 : public FilterCensusTransform<E, U8> {
    public:
        using FilterCensusTransform<E, U8>::FilterCensusTransform;

        void process( const Gray<E>& input, Gray<U8>& output ) override {
            CensusTransform::dense3x3(input, output, this->border.get());
        }
    };

    /// 5x5 census transform. Same as BoofCV's FilterCensusTransformD55S32
    template<class E>
    class FilterCensusTransformD55S32 : public FilterCensusTransform<E, S32> {
    public:
        using FilterCensusTransform<E, S32>::FilterCensusTransform;

        void process( const Gray<E>& input, Gray<S32>& output ) override {
            CensusTransform::dense5x5(input, output, this->border.get());
        }
    };

    /// Census transform which samples arbitrary points. Same as BoofCV's FilterCensusTransformSampleS64
    template<class E>
    class FilterCensusTransformSampleS64 : public FilterCensusTransform<E, S64> {
    public:
        /// @param samples Points relative to the center which are sampled. At most 64
        /// @param border How pixels outside the image are read. If null the image border isn't modified.
        FilterCensusTransformSampleS64( std::vector<Point2D<int32_t>> samples, std::shared_ptr<ImageBorder<E>> border )
                : FilterCensusTransform<E, S64>(std::move(border)), samples(std::move(samples)) {
            if (this->samples.size() > 64)
                throw std::invalid_argument("At most 64 samples");
        }

        void process( const Gray<E>& input, Gray<S64>& output ) override {
            CensusTransform::sample_S64(input, samples, output, this->border.get());
        }

    private:
        std::vector<Point2D<int32_t>> samples;
    };
}

#endif
