#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new LocalMeanBinaryFilter<U8>(region_width,(double)scale,(bool)down);
    } else {
        ptr = (jlong)new LocalMeanBinaryFilter<F32>(region_width,(double)scale,(bool)down);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (LocalMeanBinaryFilter<U8> *) nativePtr;
    } else {
        delete (LocalMeanBinaryFilter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process<LocalMeanBinaryFilter>(env, obj, jinput, joutput);
}

}