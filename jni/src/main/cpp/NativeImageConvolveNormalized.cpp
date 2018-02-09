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
        ImageAndInfo<Gray<U8>,JImageInfoU8> input = wrapGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfoU8> output = wrapGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleaseByteArrayElements((jbyteArray)input.info.jdata, input.info.data, 0);
        env->ReleaseByteArrayElements((jbyteArray)output.info.jdata, output.info.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageInfoF32> input = wrapGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfoF32> output = wrapGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        ConvolveNormalized::horizontal(*kernel, input.image,output.image);

        env->ReleaseFloatArrayElements((jfloatArray)input.info.jdata, input.info.data, 0);
        env->ReleaseFloatArrayElements((jfloatArray)output.info.jdata, output.info.data, 0);
        delete kernel;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageConvolveNormalized_nativevertical(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    if( isInteger ) {
        ImageAndInfo<Gray<U8>,JImageInfoU8> input = wrapGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfoU8> output = wrapGrayU8(env,joutput);
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleaseByteArrayElements((jbyteArray)input.info.jdata, input.info.data, 0);
        env->ReleaseByteArrayElements((jbyteArray)output.info.jdata, output.info.data, 0);
        delete kernel;
    } else {
        ImageAndInfo<Gray<F32>,JImageInfoF32> input = wrapGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfoF32> output = wrapGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);

        ConvolveNormalized::vertical(*kernel, input.image,output.image);

        env->ReleaseFloatArrayElements((jfloatArray)input.info.jdata, input.info.data, 0);
        env->ReleaseFloatArrayElements((jfloatArray)output.info.jdata, output.info.data, 0);
        delete kernel;
    }
}

}