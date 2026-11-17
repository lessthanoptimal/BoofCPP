#include <jni.h>
#include "boofcv/average_down_sample.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Calls func(input, output) with the pixels of the Java gray images. No JNI calls are allowed in 'func' since
    /// the arrays are held as critical arrays
    template<class A, class B, class Func>
    void with_critical_images( JNIEnv* env, jobject jinput, jobject joutput, Func func ) {
        ImageAndInfo<Gray<A>, JImageInfo> input = wrapCriticalGray<A>(env, jinput);
        ImageAndInfo<Gray<B>, JImageInfo> output = wrapCriticalGray<B>(env, joutput);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (A*)input_data.data;
        output.image.data = (B*)output_data.data;

        func(input.image, output.image);
    }

    /// Square regions. The output must already be the down sampled size, since a Java image can't be reshaped here
    template<class E>
    void down( JNIEnv* env, jobject jinput, jint width, jobject joutput ) {
        if (width <= 0)
            throw invalid_argument("width must be more than zero");
        with_critical_images<E, E>(env, jinput, joutput, [&]( Gray<E>& input, Gray<E>& output ) {
            if (output.width != AverageDownSampleOps::down_sample_size(input.width, (uint32_t)width) ||
                output.height != AverageDownSampleOps::down_sample_size(input.height, (uint32_t)width))
                throw invalid_argument("Output isn't the down sampled size");
            AverageDownSampleOps::down(input, (uint32_t)width, output);
        });
    }

    /// Regions with the size of the output. 'middle' must already have the output's width and the input's height
    template<class E>
    void general( JNIEnv* env, jobject jinput, jboolean centered, jobject joutput, jobject jmiddle ) {
        typedef AverageDownSampleOps::middle_type<E> F;
        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);
        ImageAndInfo<Gray<F>, JImageInfo> middle = wrapCriticalGray<F>(env, jmiddle);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        CriticalArray middle_data(env, middle.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;
        middle.image.data = (F*)middle_data.data;

        AverageDownSampleOps::down(input.image, centered, output.image, middle.image);
    }

    template<class E>
    void horizontal( JNIEnv* env, jobject jinput, jboolean centered, jobject joutput ) {
        typedef AverageDownSampleOps::middle_type<E> F;
        with_critical_images<E, F>(env, jinput, joutput, [&]( Gray<E>& input, Gray<F>& output ) {
            AverageDownSampleOps::horizontal(input, centered, output);
        });
    }

    template<class E>
    void vertical( JNIEnv* env, jobject jinput, jboolean centered, jobject joutput ) {
        typedef AverageDownSampleOps::middle_type<E> F;
        with_critical_images<F, E>(env, jinput, joutput, [&]( Gray<F>& input, Gray<E>& output ) {
            AverageDownSampleOps::vertical(input, centered, output);
        });
    }
}

// The JNI functions in NativeAverageDownSampleOps for one pixel type, e.g. nativeDownU8(). C++ exceptions become Java
// exceptions
#define BOOFCPP_JNI_CALL(call) \
    try { \
        call; \
    } catch (...) { \
        throw_java_exception(env); \
    }

#define BOOFCPP_DOWN_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeDown##T( \
        JNIEnv* env, jobject, jobject jinput, jint width, jobject joutput ) { \
    BOOFCPP_JNI_CALL(down<T>(env, jinput, width, joutput)) \
}

#define BOOFCPP_GENERAL_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeGeneral##T( \
        JNIEnv* env, jobject, jobject jinput, jboolean centered, jobject joutput, jobject jmiddle ) { \
    BOOFCPP_JNI_CALL(general<T>(env, jinput, centered, joutput, jmiddle)) \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeHorizontal##T( \
        JNIEnv* env, jobject, jobject jinput, jboolean centered, jobject joutput ) { \
    BOOFCPP_JNI_CALL(horizontal<T>(env, jinput, centered, joutput)) \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_misc_NativeAverageDownSampleOps_nativeVertical##T( \
        JNIEnv* env, jobject, jobject jinput, jboolean centered, jobject joutput ) { \
    BOOFCPP_JNI_CALL(vertical<T>(env, jinput, centered, joutput)) \
}

extern "C" {
BOOFCPP_DOWN_JNI(U8)
BOOFCPP_DOWN_JNI(S8)
BOOFCPP_DOWN_JNI(U16)
BOOFCPP_DOWN_JNI(S16)
BOOFCPP_DOWN_JNI(S32)
BOOFCPP_DOWN_JNI(F32)
BOOFCPP_DOWN_JNI(F64)

BOOFCPP_GENERAL_JNI(U8)
BOOFCPP_GENERAL_JNI(U16)
BOOFCPP_GENERAL_JNI(F32)
BOOFCPP_GENERAL_JNI(F64)
}
