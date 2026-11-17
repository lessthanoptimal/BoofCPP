#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

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
        ptr = (jlong)new GlobalFixedBinaryFilter<U8>((double)threshold, (bool)down);
    } else {
        ptr = (jlong)new GlobalFixedBinaryFilter<F32>((double)threshold, (bool)down);
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
    threshold_process<GlobalFixedBinaryFilter>(env, obj, jinput, joutput);
}

}