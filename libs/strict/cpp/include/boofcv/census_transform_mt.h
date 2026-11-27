#ifndef BOOFCPP_CENSUS_TRANSFORM_MT_H
#define BOOFCPP_CENSUS_TRANSFORM_MT_H

#include "boofcv/census_transform.h"
#include "boofcv/concurrency.h"
#include "boofcv/convolve_mt.h"

namespace boofcv {

    /// CensusTransform with the image split into blocks of rows, which threads take until none are left:
    ///
    ///     for each block of rows, concurrently:
    ///         CensusTransform's inner image code on the sub-image of the block's inner rows and the rows they read
    ///         CensusTransform's border code on the block's border pixels
    ///
    /// BoofCV's ImplCensusTransformInner_MT only splits the inner image. The border is a large part of the time, so
    /// here it's split too. Each pixel is computed by the same code as CensusTransform, so the results are identical
    /// for any number of threads
    class CensusTransform_MT {
    public:
        /// See CensusTransform::dense3x3()
        template<class E>
        static void dense3x3( const Gray<E>& input, Gray<U8>& output, ImageBorder<E>* border ) {
            dense<1>(input, output, border, [&] { CensusTransform::dense3x3(input, output, border); });
        }

        /// See CensusTransform::dense5x5()
        template<class E>
        static void dense5x5( const Gray<E>& input, Gray<S32>& output, ImageBorder<E>* border ) {
            dense<2>(input, output, border, [&] { CensusTransform::dense5x5(input, output, border); });
        }

        /// See CensusTransform::sample_S64()
        template<class E>
        static void sample_S64( const Gray<E>& input, const std::vector<Point2D<int32_t>>& samples,
                                Gray<S64>& output, ImageBorder<E>* border ) {
            if (samples.size() > 64)
                throw std::invalid_argument("At most 64 samples");
            const int32_t radius = CensusTransform::sample_radius(samples);
            output.reshape(input.width, input.height);
            rows(input, output, border, (uint32_t)radius, (uint32_t)samples.size(),
                 [&] { CensusTransform::sample_S64(input, samples, output, border); },
                 [&]( const Gray<E>& in, Gray<S64>& out ) { CensusTransform::sample_inner(in, samples, radius, out); },
                 [&]( uint32_t y0, uint32_t y1 ) {
                     CensusTransform::sample_border_rows(input, samples, radius, output, *border, y0, y1);
                 });
        }

    private:
        /// Dense census transform over the (2R+1) by (2R+1) square around each pixel
        template<int R, class E, class C, class Single>
        static void dense( const Gray<E>& input, Gray<C>& output, ImageBorder<E>* border, Single single ) {
            output.reshape(input.width, input.height);
            rows(input, output, border, R, (2*R + 1)*(2*R + 1) - 1, single,
                 []( const Gray<E>& in, Gray<C>& out ) { CensusTransform::dense_inner<R>(in, out); },
                 [&]( uint32_t y0, uint32_t y1 ) {
                     CensusTransform::dense_border_rows<R>(input, output, *border, y0, y1);
                 });
        }

        /// The loop over blocks of rows. inner(in, out) is called on sub-images and border_rows(y0, y1) only if there
        /// is a border. A comparison per sample and pixel
        template<class E, class C, class Single, class Inner, class BorderRows>
        static void rows( const Gray<E>& input, Gray<C>& output, ImageBorder<E>* border, uint32_t radius,
                          uint32_t samples, Single single, Inner inner, BorderRows border_rows ) {
            if (border != nullptr)
                border->setImage(input);
            BoofConcurrency::loop_blocks(0, input.height)
                    .element_cost((uint64_t)input.width*samples)
                    .single(single)
                    .run([&]( uint32_t y0, uint32_t y1 ) {
                        ConvolveImage_MT::inner_block(input, output, radius, radius, y0, y1, inner);
                        if (border != nullptr)
                            border_rows(y0, y1);
                    });
        }
    };
}

#endif
