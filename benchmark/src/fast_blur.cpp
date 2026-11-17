// Fast's blur against strict's, on floating point images
#include "boofcv/image_convert.h"
#include "boofcv/image_blur.h"
#include "boofcv/fast/image_blur.h"
#include "benchmark_common.h"

using namespace boofcv;

template<class E>
void bench_mean( const Gray<U8>& gray, const char* type, int N ) {
    Gray<E> input(gray.width, gray.height), output(gray.width, gray.height), storage(gray.width, gray.height);
    for (uint32_t y = 0; y < gray.height; y++)
        for (uint32_t x = 0; x < gray.width; x++)
            input.at(x, y) = gray.at(x, y);

    char name[64];
    for (uint32_t radius : {1u, 5u, 20u}) {
        snprintf(name, sizeof(name), "strict mean %s r=%u", type, radius);
        bench(name, N, [&]{ BlurImageOps::mean(input, output, radius, storage); });
        snprintf(name, sizeof(name), "fast mean %s r=%u", type, radius);
        bench(name, N, [&]{ fast::BlurImageOps::mean(input, output, radius, storage); });
    }
}

int main() {
    Interleaved<U8> input_color(1,1,1);
    if( !load_jpeg("test_image.jpg",input_color) ) {
        printf("Can't load image\n");
        return 1;
    }
    Gray<U8> gray;
    convert_average(input_color,gray);

    const int N = 20;
    bench_mean<F32>(gray, "F32", N);
    bench_mean<F64>(gray, "F64", N);
}
