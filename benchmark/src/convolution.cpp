#include "boofcv/image_convert.h"
#include "boofcv/convolve.h"
#include "benchmark_common.h"
#include <time.h>

using namespace std;
using namespace boofcv;


class BenchmarkConvolveBase {
public:
    virtual void process( const Gray<U8>&input ,
                          Gray<S16>& output,
                          uint32_t radius ) = 0;
};

class BenchmarkConvolveBase1D : public BenchmarkConvolveBase  {
public:
    void process( const Gray<U8>&input ,
                  Gray<S16>& output,
                  uint32_t radius ) override
    {
        int32_t width = radius*2 + 1;
        Kernel1D<S32> kernel = FactoryKernel::gaussian1D<S32>(-1.0,width);
        std::shared_ptr<ImageBorder<U8>> border =
                FactoryImageBorder::create_SB<U8>(BorderType::EXTENDED);
        border.get()->setImage(input);

        process(kernel,*border.get(),output);
    };

    virtual void process(Kernel1D<S32>& kernel, ImageBorder<U8>& input, Gray<S16>& output ) = 0;
};

class BenchmarkConvolveBase2D : public BenchmarkConvolveBase  {
public:
    void process( const Gray<U8>&input ,
                  Gray<S16>& output,
                  uint32_t radius ) override
    {
        int32_t width = radius*2 + 1;
        Kernel2D<S32> kernel = FactoryKernel::gaussian2D<S32>(-1.0,width);
        std::shared_ptr<ImageBorder<U8>> border =
                FactoryImageBorder::create_SB<U8>(BorderType::EXTENDED);
        border.get()->setImage(input);

        process(kernel,*border.get(),output);
    };

    virtual void process(Kernel2D<S32>& kernel, ImageBorder<U8>& input, Gray<S16>& output ) = 0;
};

class BenchmarkHorizontal : public BenchmarkConvolveBase1D {
public:
    void process(Kernel1D<S32>& kernel, ImageBorder<U8>& input, Gray<S16>& output ) override
    {
        ConvolveImage::horizontal(kernel,input,output);
    }
};

class BenchmarkVertical : public BenchmarkConvolveBase1D {
public:
    void process(Kernel1D<S32>& kernel, ImageBorder<U8>& input, Gray<S16>& output ) override
    {
        ConvolveImage::vertical(kernel,input,output);
    }
};

class BenchmarkConvolve : public BenchmarkConvolveBase2D {
public:
    void process(Kernel2D<S32>& kernel, ImageBorder<U8>& input, Gray<S16>& output ) override
    {
        ConvolveImage::convolve(kernel,input,output);
    }
};

float benchmark( BenchmarkConvolveBase& algorithm, const Gray<U8>&input , uint32_t radius )
{
    Gray<S16> output( input.width, input.height);

    clock_t t;

    t = clock();
    algorithm.process(input,output,radius);
    t = clock() - t;

    return (float)t/CLOCKS_PER_SEC;
}

float benchmark_average( int N , uint32_t radius, const Gray<U8>&input , BenchmarkConvolveBase& algorithm )
{
    float total = 0;
    for( int i = 0; i < N; i++ ) {
        total += benchmark(algorithm,input,radius);
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
    int radius=5;

    BenchmarkHorizontal horizontal;
    BenchmarkVertical vertical;
    BenchmarkConvolve convolve;

    printf("%20s radius=%2d time = %f (ms)\n","horizontal ",radius,
           benchmark_average(N,radius,gray,horizontal));
    printf("%20s radius=%2d time = %f (ms)\n","vertical ",radius,
           benchmark_average(N,radius,gray,vertical));
    printf("%20s radius=%2d time = %f (ms)\n","convolve ",radius,
           benchmark_average(N,radius,gray,convolve));

}