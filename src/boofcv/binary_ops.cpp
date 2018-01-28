#include "binary_ops.h"
#include "sanity_checks.h"

using namespace boofcv;

void boofcv::logicAnd(const Gray<U8>& inputA , const Gray<U8>& inputB , Gray<U8>& output )
{
    checkSameShape(inputA, inputB);
    checkSameShape(inputA, output);

    for( int y = 0; y < inputA.height; y++ ) {
        int indexA = inputA.offset + y*inputA.stride;
        int indexB = inputB.offset + y*inputB.stride;
        int indexOut = output.offset + y*output.stride;

        int end = indexA + inputA.width;
        for( ; indexA < end; indexA++,indexB++,indexOut++) {
            output.data[indexOut] = static_cast<U8>(inputA.data[indexA] == inputB.data[indexB]);
        }
    }
}