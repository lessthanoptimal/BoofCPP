// Benchmark for blur and convolution operations, including floating point images.
// Added during the BoofCV 1.6 update so changes to F32 code paths can be checked for speed regressions.
#include "boofcv/image_convert.h"
#include "boofcv/convolve.h"
#include "boofcv/image_blur.h"
#include "benchmark_common.h"
#include <chrono>
#include <functional>
#include <algorithm>
#include <vector>

using namespace boofcv;

// Runs the function N times and prints the mean and minimum time in milliseconds
static void bench( const char* name, int N, const std::function<void()>& f ) {
    f(); // warm up
    std::vector<double> times;
    for( int i = 0; i < N; i++ ) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        auto t1 = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double,std::milli>(t1-t0).count());
    }
    double sum = 0; for( double t : times ) sum += t;
    printf("%28s mean = %9.4f min = %9.4f (ms)\n", name, sum/N, *std::min_element(times.begin(),times.end()));
}

int main() {
    Interleaved<U8> input_color(1,1,1);
    if( !load_jpeg("test_image.jpg",input_color) ) {
        printf("Can't load image\n");
        return 1;
    }
    Gray<U8> gray;
    convert_average(input_color,gray);
    Gray<F32> grayF(gray.width,gray.height);
    for( uint32_t y = 0; y < gray.height; y++ )
        for( uint32_t x = 0; x < gray.width; x++ )
            grayF.at(x,y) = gray.at(x,y);

    const int N = 20;
    const uint32_t radius = 5;
    Gray<U8> outU8(gray.width,gray.height), storeU8(gray.width,gray.height);
    Gray<F32> outF(gray.width,gray.height), storeF(gray.width,gray.height);

    bench("mean U8 r=5", N, [&]{ BlurImageOps::mean(gray,outU8,radius,storeU8); });
    bench("mean F32 r=5", N, [&]{ BlurImageOps::mean(grayF,outF,radius,storeF); });
    bench("gaussian U8 r=5", N, [&]{ BlurImageOps::gaussian(gray,outU8,-1,radius*2+1,storeU8); });
    bench("gaussian F32 r=5", N, [&]{ BlurImageOps::gaussian(grayF,outF,-1,radius*2+1,storeF); });

    Kernel1D<F32> k1 = FactoryKernel::gaussian1D<F32>(-1,radius*2+1);
    bench("normalized horiz F32 r=5", N, [&]{ ConvolveNormalized::horizontal(k1,grayF,outF); });
    bench("normalized vert F32 r=5", N, [&]{ ConvolveNormalized::vertical(k1,grayF,outF); });

    std::shared_ptr<ImageBorder<F32>> border = FactoryImageBorder::create_SB<F32>(BorderType::EXTENDED);
    border->setImage(grayF);
    for( uint32_t r : {1u,2u,5u} ) {
        Kernel2D<F32> k2 = FactoryKernel::gaussian2D<F32>(-1,r*2+1);
        char name[64]; snprintf(name,sizeof(name),"convolve2D F32 r=%u",r);
        bench(name, N/4, [&]{ ConvolveImage::convolve(k2,*border,outF); });
    }
    return 0;
}
