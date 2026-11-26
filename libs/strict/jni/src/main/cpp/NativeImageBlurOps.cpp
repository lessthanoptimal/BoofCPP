#include <jni.h>
#include "boofcv/image_blur.h"
#include "boofcv/image_blur_mt.h"
#include <memory>
#include <type_traits>
#include <vector>
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Calls func(input, output, storage) with the pixels of the Java gray images. No JNI calls are allowed in
    /// 'func' since the arrays are held as critical arrays
    template<class E, class Func>
    void with_critical_images( JNIEnv* env, jobject jinput, jobject joutput, jobject jstorage, Func func ) {
        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);
        ImageAndInfo<Gray<E>, JImageInfo> storage = wrapCriticalGray<E>(env, jstorage);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        CriticalArray storage_data(env, storage.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;
        storage.image.data = (E*)storage_data.data;

        func(input.image, output.image, storage.image);
    }

    template<class E>
    void mean( JNIEnv* env, jobject jinput, jobject joutput, jint radius_x, jint radius_y, jobject jstorage ) {
        // Java's radii are signed
        if (radius_x < 0 || radius_y < 0)
            throw std::invalid_argument("Radius must be > 0");
        with_critical_images<E>(env, jinput, joutput, jstorage, [&]( Gray<E>& input, Gray<E>& output,
                                                                     Gray<E>& storage ) {
            if (BoofConcurrency::max_threads() > 1) {
                BlurImageOps_MT::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, storage);
                return;
            }
            BlurImageOps::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, storage);
        });
    }

    /// Copies a Java kernel. 'jdata' is an int[], float[] or double[] that matches K
    template<class K>
    Kernel1D<K> kernel_from_java( JNIEnv* env, jarray jdata, jint width, jint offset ) {
        if (width <= 0 || env->GetArrayLength(jdata) < width)
            throw invalid_argument("Bad kernel width");
        Kernel1D<K> kernel((uint32_t)width, (uint32_t)offset);
        if constexpr (std::is_same<K, S32>::value)
            env->GetIntArrayRegion((jintArray)jdata, 0, width, (jint*)kernel.data.data);
        else if constexpr (std::is_same<K, F32>::value)
            env->GetFloatArrayRegion((jfloatArray)jdata, 0, width, kernel.data.data);
        else
            env->GetDoubleArrayRegion((jdoubleArray)jdata, 0, width, kernel.data.data);
        return kernel;
    }

    /// Gaussian blur with kernels computed by BoofCV, so they're identical to the ones BoofCV's Java code uses
    template<class E>
    void gaussian( JNIEnv* env, jobject jinput, jobject joutput,
                   jarray jkernel_x, jint width_x, jint offset_x,
                   jarray jkernel_y, jint width_y, jint offset_y, jobject jstorage ) {
        typedef typename TypeInfo<E>::signed_type K;
        // Copy the kernels before acquiring critical arrays
        Kernel1D<K> kernel_x = kernel_from_java<K>(env, jkernel_x, width_x, offset_x);
        Kernel1D<K> kernel_y = kernel_from_java<K>(env, jkernel_y, width_y, offset_y);
        with_critical_images<E>(env, jinput, joutput, jstorage, [&]( Gray<E>& input, Gray<E>& output,
                                                                     Gray<E>& storage ) {
            if (BoofConcurrency::max_threads() > 1)
                BlurImageOps_MT::gaussian(input, output, kernel_x, kernel_y, storage);
            else
                BlurImageOps::gaussian(input, output, kernel_x, kernel_y, storage);
        });
    }

    template<class E>
    void mean_with_border( JNIEnv* env, jobject jinput, jobject joutput, jint radius_x, jint radius_y,
                      jobject jtype, jdouble value, jobject jstorage ) {
        // Java's radii are signed
        if (radius_x < 0 || radius_y < 0)
            throw std::invalid_argument("Radius must be > 0");
        // Look up the border before acquiring critical arrays
        std::shared_ptr<ImageBorder<E>> border = border_from_java<E>(env, jtype, value);
        std::vector<typename TypeInfo<E>::sum_type> totals;
        with_critical_images<E>(env, jinput, joutput, jstorage, [&]( Gray<E>& input, Gray<E>& output,
                                                                     Gray<E>& storage ) {
            if (BoofConcurrency::max_threads() > 1) {
                BlurImageOps_MT::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, *border, storage);
                return;
            }
            BlurImageOps::mean(input, output, (uint32_t)radius_x, (uint32_t)radius_y, *border, storage, totals);
        });
    }
}

// The JNI functions for one pixel type: nativeMean, nativeGaussian and nativeMeanBorder followed by the type, e.g.
// Java_org_boofcpp_convolve_NativeImageBlurOps_nativeMeanU8(). C++ exceptions become Java exceptions
#define BOOFCPP_BLUR_JNI(T, JKERNEL) \
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageBlurOps_nativeMean##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, jint radius_x, jint radius_y, jobject jstorage ) { \
    try { \
        mean<T>(env, jinput, joutput, radius_x, radius_y, jstorage); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageBlurOps_nativeGaussian##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, JKERNEL kernel_x, jint width_x, jint offset_x, \
        JKERNEL kernel_y, jint width_y, jint offset_y, jobject jstorage ) { \
    try { \
        gaussian<T>(env, jinput, joutput, kernel_x, width_x, offset_x, kernel_y, width_y, offset_y, jstorage); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
} \
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageBlurOps_nativeMeanBorder##T( \
        JNIEnv* env, jobject, jobject jinput, jobject joutput, jint radius_x, jint radius_y, \
        jobject jtype, jdouble value, jobject jstorage ) { \
    try { \
        mean_with_border<T>(env, jinput, joutput, radius_x, radius_y, jtype, value, jstorage); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
}

extern "C" {
BOOFCPP_BLUR_JNI(U8, jintArray)
BOOFCPP_BLUR_JNI(U16, jintArray)
BOOFCPP_BLUR_JNI(F32, jfloatArray)
BOOFCPP_BLUR_JNI(F64, jdoubleArray)
}
