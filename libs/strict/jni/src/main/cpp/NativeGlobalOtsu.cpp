#include <jni.h>
#include "boofcv/binary_ops_mt.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

/// ThresholdOps_MT, which is single threaded when BoofConcurrency::max_threads() is 1
template<class E>
using Filter = GlobalOtsuBinaryFilter<E, ThresholdOps_MT>;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativeinit(
    JNIEnv *env, jobject obj, jdouble minValue, jdouble maxValue, jdouble scale, jboolean down ) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new Filter<U8>(minValue, maxValue, (bool) down, (double)scale);
    } else {
        ptr = (jlong)new Filter<F32>(minValue, maxValue, (bool) down, (double)scale);
    }
    env->SetLongField(obj, fields.nativePtr, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativedestroy(JNIEnv *env, jobject obj) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong nativePtr = env->GetLongField(obj, fields.nativePtr);

    if( isInteger ) {
        delete (Filter<U8> *) nativePtr;
    } else {
        delete (Filter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process<Filter>(env, obj, jinput, joutput);
}

}