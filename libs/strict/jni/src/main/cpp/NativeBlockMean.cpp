#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMean_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down , jboolean thresholdFromLocalBlocks ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new ThresholdBlockMean<U8>(region_width,(bool)thresholdFromLocalBlocks,(double)scale,(bool)down);
    } else {
        ptr = (jlong)new ThresholdBlockMean<F32>(region_width,(bool)thresholdFromLocalBlocks,(double)scale,(bool)down);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMean_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (ThresholdBlockMean<U8> *) nativePtr;
    } else {
        delete (ThresholdBlockMean<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMean_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process<ThresholdBlockMean>(env, obj, jinput, joutput);
}

}