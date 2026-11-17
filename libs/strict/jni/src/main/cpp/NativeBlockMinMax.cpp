#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth,
    jdouble scale, jboolean down , jdouble minimumSpread, jboolean thresholdFromLocalBlocks ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

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
    threshold_process<ThresholdBlockMinMax>(env, obj, jinput, joutput);
}

}