#include "boofcv/fast/census_transform.h"
#include "boofcv/census_transform.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace boofcv::fast {
    namespace {
        /// Array offsets of the samples from the center pixel
        template<class E>
        std::vector<int32_t> sample_offsets( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples ) {
            std::vector<int32_t> offsets(samples.size());
            for (size_t i = 0; i < samples.size(); i++)
                offsets[i] = samples[i].y*(int32_t)input.stride + samples[i].x;
            return offsets;
        }

        /// Bits of up to 8 samples into a byte per pixel: bits[x] = sum of (center[x + offsets[k]] > center[x]) << k.
        /// Comparing 8 samples in one pass over the row writes each byte once, instead of once per sample
        template<class E>
        void byte_bits( const E* __restrict center, const int32_t* offsets, int32_t count, int32_t length,
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

        /// Inner image census from groups of 8 samples, each group's bits in a byte row, then the rows combined into
        /// the output. 'radius' is the largest x or y of a sample
        template<class E, class C>
        void byte_rows( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples, int32_t radius,
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
        void fused_5x5( const Gray<F32>& input, Gray<S32>& output ) {
            const int32_t height = input.height;
            const int32_t length = (int32_t)input.width - 4;
            if (length <= 0 || height <= 4)
                return;
            const std::vector<int32_t> offsets =
                    sample_offsets(input, boofcv::CensusTransform::create_block_samples(2, 2));
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

        template<class E>
        void dense3x3_impl( const Gray<E>& input, Gray<U8>& output, ImageBorder<E>* border ) {
            output.reshape(input.width, input.height);
            byte_rows(input, boofcv::CensusTransform::create_block_samples(1, 1), 1, output);
            if (border != nullptr)
                boofcv::CensusTransform::dense_border<1>(input, output, *border);
        }

        template<class E>
        void dense5x5_impl( const Gray<E>& input, Gray<S32>& output, ImageBorder<E>* border ) {
            output.reshape(input.width, input.height);
            if constexpr (std::is_same<E, F32>::value)
                fused_5x5(input, output);
            else
                byte_rows(input, boofcv::CensusTransform::create_block_samples(2, 2), 2, output);
            if (border != nullptr)
                boofcv::CensusTransform::dense_border<2>(input, output, *border);
        }

        template<class E>
        void sample_S64_impl( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples, Gray<S64>& output,
                              ImageBorder<E>* border ) {
            if (samples.size() > 64)
                throw std::invalid_argument("At most 64 samples");
            output.reshape(input.width, input.height);
            int32_t radius = 0;
            for (const auto& p : samples)
                radius = std::max(radius, std::max(std::abs(p.x), std::abs(p.y)));
            byte_rows(input, samples, radius, output);
            if (border != nullptr)
                boofcv::CensusTransform::sample_border(input, samples, radius, output, *border);
        }
    }

    void CensusTransform::dense3x3( const Gray<U8>& input, Gray<U8>& output, ImageBorder<U8>* border ) {
        dense3x3_impl(input, output, border);
    }
    void CensusTransform::dense3x3( const Gray<U16>& input, Gray<U8>& output, ImageBorder<U16>* border ) {
        dense3x3_impl(input, output, border);
    }
    void CensusTransform::dense3x3( const Gray<F32>& input, Gray<U8>& output, ImageBorder<F32>* border ) {
        // Byte rows were only 1.18x faster than strict, see experiments.md
        boofcv::CensusTransform::dense3x3(input, output, border);
    }
    void CensusTransform::dense5x5( const Gray<U8>& input, Gray<S32>& output, ImageBorder<U8>* border ) {
        dense5x5_impl(input, output, border);
    }
    void CensusTransform::dense5x5( const Gray<U16>& input, Gray<S32>& output, ImageBorder<U16>* border ) {
        dense5x5_impl(input, output, border);
    }
    void CensusTransform::dense5x5( const Gray<F32>& input, Gray<S32>& output, ImageBorder<F32>* border ) {
        dense5x5_impl(input, output, border);
    }
    void CensusTransform::sample_S64( const Gray<U8>& input, const std::vector<Point2D<int32_t>>& samples,
                                      Gray<S64>& output, ImageBorder<U8>* border ) {
        sample_S64_impl(input, samples, output, border);
    }
    void CensusTransform::sample_S64( const Gray<U16>& input, const std::vector<Point2D<int32_t>>& samples,
                                      Gray<S64>& output, ImageBorder<U16>* border ) {
        sample_S64_impl(input, samples, output, border);
    }
    void CensusTransform::sample_S64( const Gray<F32>& input, const std::vector<Point2D<int32_t>>& samples,
                                      Gray<S64>& output, ImageBorder<F32>* border ) {
        sample_S64_impl(input, samples, output, border);
    }
}
