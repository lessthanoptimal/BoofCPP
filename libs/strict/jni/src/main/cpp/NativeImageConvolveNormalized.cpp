#include <jni.h>
#include "boofcv/convolve.h"
#include "JNIBoofCPP.h"
#include "KernelJNI.h"

#include <memory>

using namespace boofcv;

namespace {
    /// True for GrayU8 images and an integer kernel, false for GrayF32. Set by NativeBase.setImageType()
    bool is_integer( JNIEnv* env, jobject obj ) {
        static const jfieldID fid = [env] {
            jclass local = safe_FindClass(env, "org/boofcpp/NativeBase");
            jfieldID id = safe_GetFieldID(env, local, "isInteger", "Z");
            env->NewGlobalRef(local);
            env->DeleteLocalRef(local);
            return id;
        }();
        return env->GetBooleanField(obj, fid);
    }

    /// Calls func(input, output) with the pixels of the Java gray images. No JNI calls are allowed in 'func' since
    /// the arrays are held as critical arrays
    template<class E, class Func>
    void with_critical_images( JNIEnv* env, jobject jinput, jobject joutput, Func func ) {
        ImageAndInfo<Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
        ImageAndInfo<Gray<E>, JImageInfo> output = wrapCriticalGray<E>(env, joutput);

        CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
        CriticalArray output_data(env, output.info.jdata, 0);
        input.image.data = (E*)input_data.data;
        output.image.data = (E*)output_data.data;

        func(input.image, output.image);
    }

    /// GrayU8 with a Kernel1D_S32 or GrayF32 with a Kernel1D_F32. The kernel is copied before the critical arrays
    template<class Op>
    void convolve_1d( JNIEnv* env, jobject obj, jobject jkernel, jobject jinput, jobject joutput, Op op ) {
        if (is_integer(env, obj)) {
            std::unique_ptr<Kernel1D<S32>> kernel(extractKernel1D_S32(env, jkernel));
            with_critical_images<U8>(env, jinput, joutput, [&]( Gray<U8>& in, Gray<U8>& out ) { op(*kernel, in, out); });
        } else {
            std::unique_ptr<Kernel1D<F32>> kernel(extractKernel1D_F32(env, jkernel));
            with_critical_images<F32>(env, jinput, joutput, [&]( Gray<F32>& in, Gray<F32>& out ) { op(*kernel, in, out); });
        }
    }

    void convolve_2d( JNIEnv* env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {
        if (is_integer(env, obj)) {
            std::unique_ptr<Kernel2D<S32>> kernel(extractKernel2D_S32(env, jkernel));
            with_critical_images<U8>(env, jinput, joutput, [&]( Gray<U8>& in, Gray<U8>& out ) {
                ConvolveNormalized::convolve(*kernel, in, out);
            });
        } else {
            std::unique_ptr<Kernel2D<F32>> kernel(extractKernel2D_F32(env, jkernel));
            with_critical_images<F32>(env, jinput, joutput, [&]( Gray<F32>& in, Gray<F32>& out ) {
                ConvolveNormalized::convolve(*kernel, in, out);
            });
        }
    }
}

// C++ exceptions become Java exceptions
extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativehorizontal(
        JNIEnv* env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {
    jni_call(env, [&] {
        convolve_1d(env, obj, jkernel, jinput, joutput, []( const auto& kernel, const auto& in, auto& out ) {
            ConvolveNormalized::horizontal(kernel, in, out);
        });
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativevertical(
        JNIEnv* env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {
    jni_call(env, [&] {
        convolve_1d(env, obj, jkernel, jinput, joutput, []( const auto& kernel, const auto& in, auto& out ) {
            ConvolveNormalized::vertical(kernel, in, out);
        });
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativeconvolve(
        JNIEnv* env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {
    jni_call(env, [&] { convolve_2d(env, obj, jkernel, jinput, joutput); });
}
}
