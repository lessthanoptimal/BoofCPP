#include <jni.h>
#include "boofcv/fast/binary_ops.h"
#include "boofcv/fast/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

// Fast's threshold filters for the Java classes in org.boofcpp.fast.threshold. Each class extends strict's and replaces
// its native methods

using namespace boofcv;

namespace {
    /// Creates the filter for the Java object's image type, U8 or F32, and stores it in nativePtr. 'factory' is called
    /// with a pixel of that type
    template<class Factory>
    void create( JNIEnv* env, jobject obj, Factory factory ) {
        jni_call(env, [&] {
            const NativeBaseFields& fields = NativeBaseFields::get(env);
            jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
            jlong ptr = isInteger ? (jlong)factory(U8()) : (jlong)factory(F32());
            env->SetLongField(obj, fields.nativePtr, ptr);
        });
    }

    template<template<class> class Filter>
    void destroy( JNIEnv* env, jobject obj ) {
        jni_call(env, [&] {
            const NativeBaseFields& fields = NativeBaseFields::get(env);
            jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
            jlong nativePtr = env->GetLongField(obj, fields.nativePtr);
            if (isInteger)
                delete (Filter<U8>*)nativePtr;
            else
                delete (Filter<F32>*)nativePtr;
        });
    }
}

extern "C" {
// Block mean
JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMean_nativeinit(
        JNIEnv* env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down,
        jboolean thresholdFromLocalBlocks ) {
    create(env, obj, [&]( auto pixel ) {
        return new fast::ThresholdBlockMean<decltype(pixel)>(config_from_java<ConfigLength>(env, jregionWidth),
                                                            (bool)thresholdFromLocalBlocks, (double)scale, (bool)down);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMean_nativedestroy( JNIEnv* env, jobject obj ) {
    destroy<fast::ThresholdBlockMean>(env, obj);
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMean_nativeprocess(
        JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<fast::ThresholdBlockMean>(env, obj, jinput, joutput);
}

// Block Otsu
JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockOtsu_nativeinit(
        JNIEnv* env, jobject obj, jboolean otsu2, jobject jregionWidth, jdouble tuning, jdouble scale, jboolean down,
        jboolean thresholdFromLocalBlocks ) {
    create(env, obj, [&]( auto pixel ) {
        return new fast::ThresholdBlockOtsu<decltype(pixel)>((bool)otsu2,
                                                            config_from_java<ConfigLength>(env, jregionWidth),
                                                            (double)tuning, (double)scale, (bool)down,
                                                            (bool)thresholdFromLocalBlocks);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockOtsu_nativedestroy( JNIEnv* env, jobject obj ) {
    destroy<fast::ThresholdBlockOtsu>(env, obj);
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockOtsu_nativeprocess(
        JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<fast::ThresholdBlockOtsu>(env, obj, jinput, joutput);
}

// Block min-max
JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMinMax_nativeinit(
        JNIEnv* env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down, jdouble minimumSpread,
        jboolean thresholdFromLocalBlocks ) {
    create(env, obj, [&]( auto pixel ) {
        return new fast::ThresholdBlockMinMax<decltype(pixel)>((float)minimumSpread,
                                                              config_from_java<ConfigLength>(env, jregionWidth),
                                                              (bool)thresholdFromLocalBlocks, (float)scale,
                                                              (bool)down);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMinMax_nativedestroy( JNIEnv* env, jobject obj ) {
    destroy<fast::ThresholdBlockMinMax>(env, obj);
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeBlockMinMax_nativeprocess(
        JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<fast::ThresholdBlockMinMax>(env, obj, jinput, joutput);
}
}
