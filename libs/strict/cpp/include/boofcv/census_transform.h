#ifndef BOOFCPP_CENSUS_TRANSFORM_H
#define BOOFCPP_CENSUS_TRANSFORM_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

#include "boofcv/geometry_types.h"
#include "boofcv/image_types.h"
#include "boofcv/image_border.h"

namespace boofcv {

    /// Regions sampled by the census transform. Same as BoofCV's CensusVariants
    enum class CensusVariants {
        BLOCK_3_3,
        BLOCK_5_5,
        BLOCK_7_7,
        BLOCK_9_7,
        BLOCK_13_5,
        CIRCLE_9
    };

    /// Names of CensusVariants values, in the same order. Same as the Java enum names
    inline const char* const CENSUS_VARIANTS_NAMES[] = {
            "BLOCK_3_3", "BLOCK_5_5", "BLOCK_7_7", "BLOCK_9_7", "BLOCK_13_5", "CIRCLE_9"};

    /// Number of bits in the census descriptor. Same as BoofCV's CensusVariants.getBits()
    inline uint32_t census_bits( CensusVariants variant ) {
        switch (variant) {
            case CensusVariants::BLOCK_3_3: return 3*3 - 1;
            case CensusVariants::BLOCK_5_5: return 5*5 - 1;
            case CensusVariants::BLOCK_7_7: return 7*7 - 1;
            case CensusVariants::BLOCK_9_7: return 9*7 - 1;
            case CensusVariants::BLOCK_13_5: return 13*5 - 1;
            case CensusVariants::CIRCLE_9: return 9*9 - 4*6 - 1;
        }
        throw std::invalid_argument("Unknown census variant");
    }

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
            const int32_t width = input.width;
            const int32_t height = input.height;

            // The inner image is the same distance from each edge, the largest x or y of a sample
            int32_t radius = 0;
            for (const auto& p : samples) {
                radius = std::max(radius, std::abs(p.x));
                radius = std::max(radius, std::abs(p.y));
            }

            // One pass over the row for each sample so that the loop over the row can be vectorized
            for (int32_t y = radius; y < height - radius; y++) {
                const E* __restrict center = input.data + input.offset + y*input.stride + radius;
                S64* __restrict out = output.data + output.offset + y*output.stride + radius;
                const int32_t length = width - 2*radius;
                for (int32_t x = 0; x < length; x++)
                    out[x] = 0;

                for (size_t bit = 0; bit < samples.size(); bit++) {
                    const E* __restrict neighbor = center + samples[bit].y*(int32_t)input.stride + samples[bit].x;
                    for (int32_t x = 0; x < length; x++)
                        out[x] |= (S64)((uint64_t)(neighbor[x] > center[x]) << bit);
                }
            }

            if (border == nullptr)
                return;

            border->setImage(input);

            for (int32_t y = 0; y < height; y++) {
                const bool whole_row = y < radius || y >= height - radius || width <= 2*radius;
                for (int32_t x = 0; x < width; x++) {
                    // skip the inner image
                    if (!whole_row && x == radius)
                        x = width - radius;
                    output.unsafe_at(x, y) = sample(*border, x, y, samples);
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
            const int32_t width = input.width;
            const int32_t height = input.height;

            // inner image, where the region never leaves the image. One pass over the row for each neighbor
            // so that the loop over the row can be vectorized
            for (int32_t y = R; y < height - R; y++) {
                const E* __restrict center = input.data + input.offset + y*input.stride + R;
                C* __restrict out = output.data + output.offset + y*output.stride + R;
                const int32_t length = width - 2*R;
                for (int32_t x = 0; x < length; x++)
                    out[x] = 0;

                int32_t bit = 0;
                for (int32_t dy = -R; dy <= R; dy++) {
                    for (int32_t dx = -R; dx <= R; dx++) {
                        if (dx == 0 && dy == 0)
                            continue;
                        const E* __restrict neighbor = center + dy*(int32_t)input.stride + dx;
                        for (int32_t x = 0; x < length; x++)
                            out[x] |= (C)((C)(neighbor[x] > center[x]) << bit);
                        bit++;
                    }
                }
            }

            if (border == nullptr)
                return;

            border->setImage(input);

            for (int32_t y = 0; y < height; y++) {
                const bool whole_row = y < R || y >= height - R || width <= 2*R;
                for (int32_t x = 0; x < width; x++) {
                    // skip the inner image
                    if (!whole_row && x == R)
                        x = width - R;
                    output.unsafe_at(x, y) = (C)region<R>(*border, x, y);
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
