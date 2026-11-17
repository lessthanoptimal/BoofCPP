#include <jni.h>
#include "boofcv/fast/image_blur.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Mean of the pixels of the Java gray images. They're held as critical arrays, so no JNI calls are made while
    /// the mean is computed
    template<class E>
    void mean( JNIEnv* env, jobject jinput, jobject joutput, jint radius_x, jint radius_y ) {
        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;

        Gray<E> storage(0, 0);
        fast::BlurImageOps::mean(input.image, output.image, (uint32_t)radius_x, (uint32_t)radius_y, storage);
    }
}

extern "C" {

// C++ exceptions become Java exceptions
#define BOOFCPP_FAST_MEAN_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_fast_convolve_NativeImageBlurOps_nativeMean##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, jint radius_x, jint radius_y ) { \
    try { \
        mean<T>(env, jinput, joutput, radius_x, radius_y); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
}

BOOFCPP_FAST_MEAN_JNI(F32)
BOOFCPP_FAST_MEAN_JNI(F64)

}
