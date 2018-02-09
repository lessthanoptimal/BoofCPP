#include <jni.h>
#include <binary_ops.h>
#include <threshold_block_filters.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativeinit(
    JNIEnv *env, jobject obj, jboolean otsu2, jobject jregionWidth, jdouble tuning,
    jdouble scale, jboolean down , jboolean thresholdFromLocalBlocks ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = extractConfigLength(env,jregionWidth);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new ThresholdBlockOtsu<U8>((bool)otsu2,region_width,(double)tuning,(double)scale,(bool)down,(bool)thresholdFromLocalBlocks);
    } else {
        ptr = (jlong)new ThresholdBlockOtsu<F32>((bool)otsu2,region_width,(double)tuning,(double)scale,(bool)down,(bool)thresholdFromLocalBlocks);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (ThresholdBlockOtsu<U8> *) nativePtr;
    } else {
        delete (ThresholdBlockOtsu<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {

    // Get the pointer to the thresholding algorithm
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    if( env->ExceptionCheck() )
        return;
    jlong nativePtr = env->GetLongField(obj, fid);
    if( env->ExceptionCheck() )
            return;

    JImageInfoU8 outputInfo = extractInfoU8(env,joutput);

    Gray<U8> output((U8*)outputInfo.data,(uint32_t)outputInfo.dataLength,
                   (uint32_t)outputInfo.width,(uint32_t)outputInfo.height,
                   (uint32_t)outputInfo.offset,(uint32_t)outputInfo.stride);

    try {
//        printf("   alg.threshold %d\n",alg->threshold);
        if( isInteger ) {
            ImageAndInfo<Gray<U8>,JImageInfoU8> input = wrapGrayU8(env,jinput);
            ((ThresholdBlockOtsu<U8>*)nativePtr)->process(input.image, output);
            env->ReleaseByteArrayElements((jbyteArray)input.info.jdata, input.info.data, 0);
        } else {
            ImageAndInfo<Gray<F32>,JImageInfoF32> input = wrapGrayF32(env,jinput);
            ((ThresholdBlockOtsu<F32> *)nativePtr)->process(input.image, output);
            env->ReleaseFloatArrayElements((jfloatArray)input.info.jdata, input.info.data, 0);
        }
    } catch( ... ) {
        printf("Exception!!\n");
        // TODO do something here
    }

    // Release the arrays
    env->ReleaseByteArrayElements((jbyteArray)outputInfo.jdata, outputInfo.data, 0);
}

}