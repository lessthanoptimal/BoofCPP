#include <jni.h>
#include "boofcv/binary_ops_mt.h"
#include "boofcv/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativeinit(
    JNIEnv *env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down ) {
    jni_call(env, [&] {
        const NativeBaseFields& fields = NativeBaseFields::get(env);
        jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);

        boofcv::ConfigLength region_width = config_from_java<boofcv::ConfigLength>(env,jregionWidth);

        jlong ptr;
        if( isInteger ) {
            ptr = (jlong)new LocalMeanBinaryFilter<U8>(region_width,(double)scale,(bool)down);
        } else {
            ptr = (jlong)new LocalMeanBinaryFilter<F32>(region_width,(double)scale,(bool)down);
        }
        env->SetLongField(obj, fields.nativePtr, ptr);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativedestroy(JNIEnv *env, jobject obj) {

    const NativeBaseFields& fields = NativeBaseFields::get(env);
    jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
    jlong nativePtr = env->GetLongField(obj, fields.nativePtr);

    if( isInteger ) {
        delete (LocalMeanBinaryFilter<U8> *) nativePtr;
    } else {
        delete (LocalMeanBinaryFilter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeLocalMean_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {
    threshold_process_with<LocalMeanBinaryFilter, ThresholdOps_MT>(env, obj, jinput, joutput);
}

}