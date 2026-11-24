#include <jni.h>
#include "boofcv/fast/image_blur.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Calls func(input, output) with the pixels of the Java gray images. They're held as critical arrays, so 'func'
    /// can't make JNI calls
    template<class E, class Func>
    void with_images( JNIEnv* env, jobject jinput, jobject joutput, Func func ) {
        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;
        func(input.image, output.image);
    }

    template<class E>
    void mean( JNIEnv* env, jobject jinput, jobject joutput, jint radius_x, jint radius_y ) {
        // Java's radii are signed
        if (radius_x < 0 || radius_y < 0)
            throw std::invalid_argument("Radius must be > 0");
        with_images<E>(env, jinput, joutput, [&]( Gray<E>& input, Gray<E>& output ) {
            Gray<E> storage(0, 0);
            fast::BlurImageOps::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, storage);
        });
    }

    template<class E>
    void mean_with_border( JNIEnv* env, jobject jinput, jobject joutput, jint radius_x, jint radius_y, jobject jtype,
                      jdouble value ) {
        // Java's radii are signed
        if (radius_x < 0 || radius_y < 0)
            throw std::invalid_argument("Radius must be > 0");
        // Look up the border before acquiring critical arrays
        std::shared_ptr<ImageBorder<E>> border = border_from_java<E>(env, jtype, value);
        with_images<E>(env, jinput, joutput, [&]( Gray<E>& input, Gray<E>& output ) {
            Gray<E> storage(0, 0);
            std::vector<E> totals;
            fast::BlurImageOps::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, *border, storage, totals);
        });
    }
}

extern "C" {

// The JNI functions for one pixel type, e.g. Java_org_boofcpp_fast_convolve_NativeImageBlurOps_nativeMeanF32()
#define BOOFCPP_FAST_MEAN_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_fast_convolve_NativeImageBlurOps_nativeMean##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, jint radius_x, jint radius_y ) { \
    jni_call(env, [&] { mean<T>(env, jinput, joutput, radius_x, radius_y); }); \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_fast_convolve_NativeImageBlurOps_nativeMeanBorder##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, jint radius_x, jint radius_y, jobject jtype, \
        jdouble value ) { \
    jni_call(env, [&] { mean_with_border<T>(env, jinput, joutput, radius_x, radius_y, jtype, value); }); \
}

BOOFCPP_FAST_MEAN_JNI(F32)
BOOFCPP_FAST_MEAN_JNI(F64)

}
