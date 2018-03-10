#include <jni.h>
#include <convolve.h>
#include "JNIBoofCPP.h"

#include <stdio.h>

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativehorizontal(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    if( isInteger ) {
        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfo> output = wrapCriticalGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativevertical(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    if( isInteger ) {
        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfo> output = wrapCriticalGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    }
}

}