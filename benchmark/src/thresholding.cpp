#include <boofcv/binary_ops.h>
#include "boofcv/image_convert.h"
#include "boofcv/threshold_block_filters.h"
#include "benchmark_common.h"
#include <time.h>

using namespace std;
using namespace boofcv;

float benchmark( const Gray<U8>&input , InputToBinary<Gray<U8>>& algorithm )
{
    Gray<U8> output( input.width, input.height);

    clock_t t;

    t = clock();
    algorithm.process(input,output);
    t = clock() - t;

    return (float)t/CLOCKS_PER_SEC;
}

float benchmark_average( int N , const Gray<U8>&input , InputToBinary<Gray<U8>>& algorithm )
{
    float total = 0;
    for( int i = 0; i < N; i++ ) {
        total += benchmark(input,algorithm);
    }

    return 1000.0f*total/N;
}


int main() {
    Interleaved<U8> input_color(1,1,1);

    if( !load_jpeg("test_image.jpg",input_color) ) {
        printf("Can't load image\n");
        return 1;
    }

    Gray<U8> gray;

    convert_average(input_color,gray);

    int N = 10;
    bool down = true;
    float scale = 0.95f;
    ConfigLength regionWidth = ConfigLength::fixed(40);
    bool local_blocks = false;

    GlobalFixedBinaryFilter<U8> global_fixed(125,down);
    GlobalOtsuBinaryFilter<U8> global_otsu(0,255,down);
    LocalMeanBinaryFilter<U8> local_mean(regionWidth,scale,down);
    ThresholdBlockMinMax<U8> block_min_max(1,regionWidth,local_blocks,scale,down);
    ThresholdBlockMean<U8> block_mean(regionWidth,local_blocks,scale,down);
    ThresholdBlockOtsu<U8> block_otsu(true,regionWidth,0,scale,down,local_blocks);

    printf("%20s time = %f (ms)\n","global_fixed",benchmark_average(N,gray,global_fixed));
    printf("%20s time = %f (ms)\n","global_otsu",benchmark_average(N,gray,global_otsu));
    printf("%20s time = %f (ms)\n","local_mean",benchmark_average(N,gray,local_mean));
    printf("%20s time = %f (ms)\n","block_min_max",benchmark_average(N,gray,block_min_max));
    printf("%20s time = %f (ms)\n","block_mean",benchmark_average(N,gray,block_mean));
    printf("%20s time = %f (ms)\n","block_otsu",benchmark_average(N,gray,block_otsu));
}