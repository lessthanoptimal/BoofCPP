#include <jni.h>
#include "boofcv/binary_ops_mt.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

/// ThresholdOps_MT, which is single threaded when BoofConcurrency::max_threads() is 1
template<class E>
using Filter = GlobalFixedBinaryFilter<E, ThresholdOps_MT>;

// double threshold, boolean down
extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativeinit(
    JNIEnv *env, jobject obj, jdouble threshold, jboolean down ) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);

    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new Filter<U8>((double)threshold, (bool)down);
    } else {
        ptr = (jlong)new Filter<F32>((double)threshold, (bool)down);
    }
    env->SetLongField(obj, fields.nativePtr, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativedestroy(JNIEnv *env, jobject obj) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong nativePtr = env->GetLongField(obj, fields.nativePtr);

    if( isInteger ) {
        delete (Filter<U8> *) nativePtr;
    } else {
        delete (Filter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process<Filter>(env, obj, jinput, joutput);
}

}