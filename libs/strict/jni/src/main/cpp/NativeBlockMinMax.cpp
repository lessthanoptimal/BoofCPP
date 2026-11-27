#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/threshold_block_filters_mt.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

template<class E>
using Filter = ThresholdBlock_MT<ThresholdBlockMinMax<E>>;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth,
    jdouble scale, jboolean down , jdouble minimumSpread, jboolean thresholdFromLocalBlocks ) {
    jni_call(env, [&] {
        const NativeBaseFields& fields = NativeBaseFields::get(env);
        jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);

        boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

        jlong ptr;
        if( isInteger ) {
            ptr = (jlong)new Filter<U8>(minimumSpread,region_width,(bool)thresholdFromLocalBlocks,scale,(bool)down);
        } else {
            ptr = (jlong)new Filter<F32>(minimumSpread,region_width,(bool)thresholdFromLocalBlocks,scale,(bool)down);
        }
        env->SetLongField(obj, fields.nativePtr, ptr);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativedestroy(JNIEnv *env, jobject obj) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong nativePtr = env->GetLongField(obj, fields.nativePtr);

    if( isInteger ) {
        delete (Filter<U8> *) nativePtr;
    } else {
        delete (Filter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeBlockMinMax_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process_block<Filter>(env, obj, jinput, joutput);
}

}