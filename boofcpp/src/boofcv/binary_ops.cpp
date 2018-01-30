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