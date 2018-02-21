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
        ImageAndInfo<Gray<U8>,JImageCritical> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageCritical> output = wrapCriticalGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageCritical> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageCritical> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        delete kernel;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativevertical(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    if( isInteger ) {
        ImageAndInfo<Gray<U8>,JImageCritical> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageCritical> output = wrapCriticalGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageCritical> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageCritical> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        delete kernel;
    }
}

}