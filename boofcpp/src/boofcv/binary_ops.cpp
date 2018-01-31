#include <algorithm>

#include "binary_ops.h"
#include "sanity_checks.h"

using namespace boofcv;

void boofcv::logicAnd(const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output )
{
    checkSameShape(inputA, inputB);
    output.reshape(inputA.width,inputA.height);

    for( int y = 0; y < inputA.height; y++ ) {
        int indexA = inputA.offset + y*inputA.stride;
        int indexB = inputB.offset + y*inputB.stride;
        int indexOut = output.offset + y*output.stride;

        int end = indexA + inputA.width;
        for( ; indexA < end; indexA++,indexB++,indexOut++) {
            output.data[indexOut] = static_cast<U8>(inputA.data[indexA] && inputB.data[indexB]);
        }
    }
}

void boofcv::logicOr(const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output )
{
    checkSameShape(inputA, inputB);
    output.reshape(inputA.width,inputA.height);

    for( int y = 0; y < inputA.height; y++ ) {
        int indexA = inputA.offset + y*inputA.stride;
        int indexB = inputB.offset + y*inputB.stride;
        int indexOut = output.offset + y*output.stride;

        int end = indexA + inputA.width;
        for( ; indexA < end; indexA++,indexB++,indexOut++) {
            output.data[indexOut] = static_cast<U8>(inputA.data[indexA] || inputB.data[indexB]);
        }
    }
}

void boofcv::erode4(const Gray<U8>& input, Gray<U8>& output) {
    output.reshape(input.width,input.height);

    const int h = input.height - 1;
    const int w = input.width - 2;

    for (int y = 1; y < h; y++) {
        int indexIn = input.offset + y * input.stride + 1;
        int indexOut = output.offset + y * output.stride + 1;

        const int end = indexIn + w;

        for (; indexIn < end; indexIn++, indexOut++) {
            if ((input.data[indexIn] + input.data[indexIn - 1] + input.data[indexIn + 1] +
                 input.data[indexIn - input.stride] + input.data[indexIn + input.stride]) == 5)
                output.data[indexOut] = 1;
            else
                output.data[indexOut] = 0;
        }
    }
}

void boofcv::dilate4(const Gray<U8>& input, Gray<U8>& output) {
    output.reshape(input.width,input.height);

    const int h = input.height - 1;
    const int w = input.width - 2;

    for (int y = 1; y < h; y++) {
        int indexIn = input.offset + y * input.stride + 1;
        int indexOut = output.offset + y * output.stride + 1;

        const int end = indexIn + w;

        for (; indexIn < end; indexIn++, indexOut++) {
            if ((input.data[indexIn] +
                 input.data[indexIn - 1] + input.data[indexIn + 1] +
                 input.data[indexIn - input.stride] + input.data[indexIn + input.stride]) > 0)
                output.data[indexOut] = 1;
            else
                output.data[indexOut] = 0;
        }
    }
}

boofcv::ComputeOtsu::ComputeOtsu(bool useOtsu2, double tuning, bool down, double scale) {
    this->useOtsu2 = useOtsu2;
    this->tuning = tuning;
    this->down = down;
    this->scale = scale;
}

boofcv::ComputeOtsu::ComputeOtsu( bool useOtsu2 , bool down ) : ComputeOtsu(useOtsu2,0,down,1.0) {
}

void boofcv::ComputeOtsu::compute(const GrowArray<uint32_t>&histogram , uint32_t totalPixels) {
    if( useOtsu2 ) {
        computeOtsu2(histogram,totalPixels);
    } else {
        computeOtsu(histogram,totalPixels);
    }

    // apply optional penalty to low texture regions
    variance += 0.001; // avoid divide by zero
    // multiply by threshold twice in an effort to have the image's scaling not effect the tuning parameter
    int adjustment =  (int)(tuning*threshold*tuning*threshold/variance+0.5);
    threshold += down ? -adjustment : adjustment;
    threshold = (int)(scale*std::max(threshold,0.0)+0.5);
}

void boofcv::ComputeOtsu::computeOtsu(const GrowArray<uint32_t>& histogram , uint32_t totalPixels ) {
    double dlength = histogram.size;
    double sum = 0;
    for (int i = 0; i < histogram.size; i++)
        sum += (i / dlength) * histogram[i];

    double sumB = 0;
    int wB = 0;

    variance = 0;
    threshold = 0;

    int i;
    for (i = 0; i < histogram.size; i++) {
        wB += histogram[i];               // Weight Background
        if (wB == 0) continue;

        int wF = totalPixels - wB;        // Weight Foreground
        if (wF == 0) break;

        double f = i / dlength;
        sumB += f * histogram[i];

        double mB = sumB / wB;            // Mean Background
        double mF = (sum - sumB) / wF;    // Mean Foreground

        // Calculate Between Class Variance
        double varBetween = (double) wB * (double) wF * (mB - mF) * (mB - mF);

        // Check if new maximum found
        if (varBetween > variance) {
            variance = varBetween;
            threshold = i;
        }
    }
}

void boofcv::ComputeOtsu::computeOtsu2(const GrowArray<uint32_t>&histogram , uint32_t totalPixels ) {
    double dlength = histogram.size;
    double sum = 0;
    for (int i = 0; i < histogram.size; i++)
        sum += (i / dlength) * histogram[i];

    double sumB = 0;
    int wB = 0;

    variance = 0;
    threshold = 0;

    double selectedMB=0;
    double selectedMF=0;

    int i;
    for (i = 0; i < histogram.size; i++) {
        wB += histogram[i];               // Weight Background
        if (wB == 0) continue;

        int wF = totalPixels - wB;        // Weight Foreground
        if (wF == 0) break;

        double f = i / dlength;
        sumB += f * histogram[i];

        double mB = sumB / wB;            // Mean Background
        double mF = (sum - sumB) / wF;    // Mean Foreground

        // Calculate Between Class Variance
        double varBetween = (double) wB * (double) wF * (mB - mF) * (mB - mF);

        // Check if new maximum found
        if (varBetween > variance) {
            variance = varBetween;
            selectedMB = mB;
            selectedMF = mF;
        }
    }

    // select a threshold which maximizes the distance between the two distributions. In pathological
    // cases there's a dead zone where all the values are equally good and it would select a value with a low index
    // arbitrarily. Then if you scaled the threshold it would reject everything
    threshold = histogram.size*(selectedMB+selectedMF)/2.0;
}