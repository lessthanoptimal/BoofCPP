// Benchmark for stereo block matching. Same input and configuration as the JMH DisparityBlockMatchBenchmark:
// the test image downsampled by 6 (672x504), the right image is the left shifted by 16 pixels plus noise, and the
// default ConfigDisparityBM (range 100, radius 3, subpixel, right to left 1, texture 0.15, REFLECT, score saved)
// with only the error type changed.
#include "boofcv/average_down_sample.h"
#include "boofcv/image_convert.h"
#include "boofcv/stereo_disparity.h"
#include "benchmark_common.h"
#include <algorithm>
#include <random>
#include <vector>

using namespace boofcv;

int main() {
    Interleaved<U8> input_color(1,1,1);
    if( !load_jpeg("test_image.jpg",input_color) ) {
        printf("Can't load image\n");
        return 1;
    }
    Gray<U8> gray, left;
    convert_average(input_color,gray);
    AverageDownSampleOps::down(gray, 6, left);

    const int32_t shift = 16;
    Gray<U8> right(left.width, left.height);
    std::mt19937 gen(0xBEEF);
    std::uniform_int_distribution<int32_t> noise(-3, 3);
    for( uint32_t y = 0; y < left.height; y++ ) {
        for( uint32_t x = 0; x < left.width; x++ ) {
            int32_t v = left.at(std::min<uint32_t>(left.width - 1, x + shift), y) + noise(gen);
            right.at(x,y) = (U8)std::max(0, std::min(255, v));
        }
    }

    ConfigDisparityBM config;
    printf("image %ux%u range %d radius %d\n", left.width, left.height, config.disparity_range, config.region_radius_x);

    auto run = [&]( const char* name, DisparityError error, CensusVariants variant ) {
        config.error_type = error;
        config.config_census.variant = variant;
        auto alg = FactoryStereoDisparity::block_match<U8, F32>(config);
        bench(name, 20, [&]{ alg->process(left, right); });

        // Sanity check, most valid pixels should be at the shift
        const Gray<F32>& disparity = alg->get_disparity();
        uint32_t valid = 0, near = 0;
        for( uint32_t i = 0; i < disparity.data_length; i++ ) {
            float d = disparity.data[i];
            if( d < config.disparity_range ) {
                valid++;
                if( std::abs(d - shift) <= 1 )
                    near++;
            }
        }
        printf("  valid %.1f%%, of which at %d+-1 %.1f%%\n", 100.0*valid/disparity.data_length, shift,
               100.0*near/std::max(1u,valid));
    };
    run("sad U8 subpixel", DisparityError::SAD, CensusVariants::BLOCK_5_5);
    run("census 5x5 subpixel", DisparityError::CENSUS, CensusVariants::BLOCK_5_5);
    run("census 3x3 subpixel", DisparityError::CENSUS, CensusVariants::BLOCK_3_3);
    run("census 7x7 subpixel", DisparityError::CENSUS, CensusVariants::BLOCK_7_7);
    run("census 9x7 subpixel", DisparityError::CENSUS, CensusVariants::BLOCK_9_7);
    run("census 13x5 subpixel", DisparityError::CENSUS, CensusVariants::BLOCK_13_5);
    run("census circle 9 subpixel", DisparityError::CENSUS, CensusVariants::CIRCLE_9);
    run("ssd U8 subpixel", DisparityError::SSD, CensusVariants::BLOCK_5_5);
    run("zssd U8 subpixel", DisparityError::ZSSD, CensusVariants::BLOCK_5_5);
    run("ncc U8 subpixel", DisparityError::NCC, CensusVariants::BLOCK_5_5);

    Gray<F32> left_f, right_f;
    convert(left, left_f);
    convert(right, right_f);
    auto run_f32 = [&]( const char* name, DisparityError error ) {
        config.error_type = error;
        auto alg = FactoryStereoDisparity::block_match<F32, F32>(config);
        bench(name, 20, [&]{ alg->process(left_f, right_f); });
    };
    run_f32("sad F32 subpixel", DisparityError::SAD);
    run_f32("ssd F32 subpixel", DisparityError::SSD);
    run_f32("zssd F32 subpixel", DisparityError::ZSSD);
    run_f32("ncc F32 subpixel", DisparityError::NCC);
    run_f32("census 5x5 F32 subpixel", DisparityError::CENSUS);

    auto border = FactoryImageBorder::create_SB<U8>(REFLECT);
    Gray<S32> census5(1, 1);
    Gray<U8> census3(1, 1);
    bench("census transform 5x5", 20, [&]{ CensusTransform::dense5x5(left, census5, border.get()); });
    bench("census transform 3x3", 20, [&]{ CensusTransform::dense3x3(left, census3, border.get()); });
    Gray<S64> census64(1, 1);
    auto samples7 = CensusTransform::create_block_samples(3, 3);
    auto samples13 = CensusTransform::create_block_samples(6, 2);
    bench("census transform 7x7", 20, [&]{ CensusTransform::sample_S64(left, samples7, census64, border.get()); });
    bench("census transform 13x5", 20, [&]{ CensusTransform::sample_S64(left, samples13, census64, border.get()); });
    return 0;
}
