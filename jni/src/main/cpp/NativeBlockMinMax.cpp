#include <jni.h>
#include <binary_ops.h>
#include <threshold_block_filters.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth,
    jdouble scale, jboolean down , jdouble minimumSpread, jboolean thresholdFromLocalBlocks ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = extractConfigLength(env,jregionWidth);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new ThresholdBlockMinMax<U8>((float)minimumSpread,region_width,(bool)thresholdFromLocalBlocks,(float)scale,(bool)down);
    } else {
        ptr = (jlong)new ThresholdBlockMinMax<F32>((float)minimumSpread,region_width,(bool)thresholdFromLocalBlocks,(float)scale,(bool)down);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (ThresholdBlockMinMax<U8> *) nativePtr;
    } else {
        delete (ThresholdBlockMinMax<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativeprocess
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
            JImageInfoU8 inputInfo = extractInfoU8(env,jinput);
            Gray<U8> input((U8*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                           (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                           (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);
            ((ThresholdBlockMinMax<U8>*)nativePtr)->process(input, output);
            env->ReleaseByteArrayElements((jbyteArray)inputInfo.jdata, inputInfo.data, 0);
        } else {
            JImageInfoF32 inputInfo = extractInfoF32(env,jinput);
            Gray<F32> input((F32*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                           (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                           (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);
            ((ThresholdBlockMinMax<F32> *)nativePtr)->process(input, output);
            env->ReleaseFloatArrayElements((jfloatArray)inputInfo.jdata, inputInfo.data, 0);
        }
    } catch( ... ) {
        printf("Exception!!\n");
        // TODO do something here
    }

    // Release the arrays
    env->ReleaseByteArrayElements((jbyteArray)outputInfo.jdata, outputInfo.data, 0);
}

}