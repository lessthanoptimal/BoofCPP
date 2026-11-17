#include <jni.h>
#include <average_down_sample.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// The output must already be the down sampled size, since a Java image can't be reshaped from here
    template<class E>
    void down( JNIEnv* env, jobject jinput, jint width, jobject joutput ) {
        if (width <= 0)
            throw invalid_argument("width must be more than zero");

        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);
        if (output.image.width != AverageDownSampleOps::down_sample_size(input.image.width, (uint32_t)width) ||
            output.image.height != AverageDownSampleOps::down_sample_size(input.image.height, (uint32_t)width))
            throw invalid_argument("Output isn't the down sampled size");

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;

        AverageDownSampleOps::down(input.image, (uint32_t)width, output.image);
    }
}

// Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeDown followed by the type, e.g. nativeDownU8(). C++
// exceptions become Java exceptions
#define BOOFCPP_DOWN_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeDown##T( \
        JNIEnv* env, jobject, jobject jinput, jint width, jobject joutput ) { \
    try { \
        down<T>(env, jinput, width, joutput); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
}

extern "C" {
BOOFCPP_DOWN_JNI(U8)
BOOFCPP_DOWN_JNI(S8)
BOOFCPP_DOWN_JNI(U16)
BOOFCPP_DOWN_JNI(S16)
BOOFCPP_DOWN_JNI(S32)
BOOFCPP_DOWN_JNI(F32)
BOOFCPP_DOWN_JNI(F64)
}
