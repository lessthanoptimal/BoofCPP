#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters_mt.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

/// ThresholdBlock_MT, which is single threaded when BoofConcurrency::max_threads() is 1
template<class E>
using Filter = ThresholdBlock_MT<ThresholdBlockOtsu<E>>;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativeinit(
    JNIEnv *env, jobject obj, jboolean otsu2, jobject jregionWidth, jdouble tuning,
    jdouble scale, jboolean down , jboolean thresholdFromLocalBlocks ) {
    jni_call(env, [&] {
        const NativeBaseFields& fields = NativeBaseFields::get(env);
        jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);

        boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

        jlong ptr;
        if( isInteger ) {
            ptr = (jlong)new Filter<U8>((bool)otsu2,region_width,(double)tuning,(double)scale,(bool)down,(bool)thresholdFromLocalBlocks);
        } else {
            ptr = (jlong)new Filter<F32>((bool)otsu2,region_width,(double)tuning,(double)scale,(bool)down,(bool)thresholdFromLocalBlocks);
        }
        env->SetLongField(obj, fields.nativePtr, ptr);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativedestroy(JNIEnv *env, jobject obj) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong nativePtr = env->GetLongField(obj, fields.nativePtr);

    if( isInteger ) {
        delete (Filter<U8> *) nativePtr;
    } else {
        delete (Filter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockOtsu_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process<Filter>(env, obj, jinput, joutput);
}

}