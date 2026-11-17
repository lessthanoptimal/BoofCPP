#include <jni.h>
#include "boofcv/census_transform.h"
#include <memory>
#include <vector>
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Calls func(input, output, border) with the pixels of the Java gray images. 'border' is null if there's no
    /// border. No JNI calls are allowed in 'func' since the arrays are held as critical arrays
    template<class E, class C, class Func>
    void with_images( JNIEnv* env, jobject jinput, jobject joutput, jboolean has_border, jobject jtype,
                      jdouble value, Func func ) {
        // Look up the border before acquiring critical arrays
        std::shared_ptr<ImageBorder<E>> border;
        if (has_border)
            border = border_from_java<E>(env, jtype, value);

        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<C>, JImageInfo> output = wrapCriticalGray<C>(env, joutput);
        if (input.image.width != output.image.width || input.image.height != output.image.height)
            throw invalid_argument("Output isn't the input's shape");

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (C*)output_data.data;
        func(input.image, output.image, border.get());
    }

    template<class E>
    void dense( JNIEnv* env, jint width, jobject jinput, jobject joutput, jboolean has_border, jobject jtype,
                jdouble value ) {
        if (width == 3) {
            with_images<E, U8>(env, jinput, joutput, has_border, jtype, value,
                               []( Gray<E>& input, Gray<U8>& output, ImageBorder<E>* border ) {
                                   CensusTransform::dense3x3(input, output, border);
                               });
        } else if (width == 5) {
            with_images<E, S32>(env, jinput, joutput, has_border, jtype, value,
                                []( Gray<E>& input, Gray<S32>& output, ImageBorder<E>* border ) {
                                    CensusTransform::dense5x5(input, output, border);
                                });
        } else {
            throw invalid_argument("Width must be 3 or 5");
        }
    }

    template<class E>
    void sample_s64( JNIEnv* env, jobject jinput, jintArray jxs, jintArray jys, jobject joutput,
                     jboolean has_border, jobject jtype, jdouble value ) {
        // Copy the samples before acquiring critical arrays
        const jsize count = env->GetArrayLength(jxs);
        if (env->GetArrayLength(jys) != count)
            throw invalid_argument("Different number of x and y");
        std::vector<jint> xs(count), ys(count);
        env->GetIntArrayRegion(jxs, 0, count, xs.data());
        env->GetIntArrayRegion(jys, 0, count, ys.data());
        std::vector<Point2D<int32_t>> samples(count);
        for (jsize i = 0; i < count; i++)
            samples[i] = Point2D<int32_t>(xs[i], ys[i]);

        with_images<E, S64>(env, jinput, joutput, has_border, jtype, value,
                            [&]( Gray<E>& input, Gray<S64>& output, ImageBorder<E>* border ) {
                                CensusTransform::sample_S64(input, samples, output, border);
                            });
    }
}

// The JNI functions for one input pixel type, e.g. Java_org_boofcpp_transform_NativeCensusTransform_nativeDenseU8().
// C++ exceptions become Java exceptions
#define BOOFCPP_CENSUS_JNI(T) \
JNIEXPORT void JNICALL Java_org_boofcpp_transform_NativeCensusTransform_nativeDense##T( \
        JNIEnv* env, jclass, jint width, jobject jinput, jobject joutput, jboolean has_border, jobject jtype, \
        jdouble value ) { \
    jni_call(env, [&] { dense<T>(env, width, jinput, joutput, has_border, jtype, value); }); \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_transform_NativeCensusTransform_nativeSampleS64##T( \
        JNIEnv* env, jclass, jobject jinput, jintArray jxs, jintArray jys, jobject joutput, jboolean has_border, \
        jobject jtype, jdouble value ) { \
    jni_call(env, [&] { sample_s64<T>(env, jinput, jxs, jys, joutput, has_border, jtype, value); }); \
}

extern "C" {
BOOFCPP_CENSUS_JNI(U8)
BOOFCPP_CENSUS_JNI(U16)
BOOFCPP_CENSUS_JNI(F32)
}
