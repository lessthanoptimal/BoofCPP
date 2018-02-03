#include <jni.h>
#include <binary_ops.h>
#include <threshold_block_filters.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

// double threshold, boolean down
extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativeinit(
    JNIEnv *env, jobject obj, jdouble threshold, jboolean down ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new GlobalFixedBinaryFilter<U8>((U8)threshold, (bool)down);
    } else {
        ptr = (jlong)new GlobalFixedBinaryFilter<F32>((float)threshold, (bool)down);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (GlobalFixedBinaryFilter<U8> *) nativePtr;
    } else {
        delete (GlobalFixedBinaryFilter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativeprocess
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
            ((GlobalFixedBinaryFilter<U8>*)nativePtr)->process(input, output);
            env->ReleaseByteArrayElements((jbyteArray)inputInfo.jdata, inputInfo.data, 0);
        } else {
            JImageInfoF32 inputInfo = extractInfoF32(env,jinput);
            Gray<F32> input((F32*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                           (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                           (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);
            ((GlobalFixedBinaryFilter<F32> *)nativePtr)->process(input, output);
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