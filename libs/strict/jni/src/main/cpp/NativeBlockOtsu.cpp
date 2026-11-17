#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativeinit(
    JNIEnv *env, jobject obj, jboolean otsu2, jobject jregionWidth, jdouble tuning,
    jdouble scale, jboolean down , jboolean thresholdFromLocalBlocks ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

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
    threshold_process<ThresholdBlockOtsu>(env, obj, jinput, joutput);
}

}