#include <jni.h>
#include "boofcv/fast/binary_ops.h"
#include "boofcv/fast/threshold_block_filters.h"
#include "JNIBoofCPP.h"
#include "ThresholdJNI.h"

// Fast's threshold filters for the Java classes in org.boofcpp.fast.threshold. Each class extends strict's and replaces
// its native methods. Strict's ThresholdJNI.h wraps the images

using namespace boofcv;

namespace {
    /// Creates the filter for the Java object's image type, U8 or F32, and stores it in nativePtr. 'factory' is called
    /// with a pixel of that type
    template<class Factory>
    void create( JNIEnv* env, jobject obj, Factory factory ) {
        jni_call(env, [&] {
            jclass objClass = env->GetObjectClass(obj);
            jboolean isInteger = env->GetBooleanField(obj, safe_GetFieldID(env, objClass, "isInteger", "Z"));
            jlong ptr = isInteger ? (jlong)factory(U8()) : (jlong)factory(F32());
            env->SetLongField(obj, safe_GetFieldID(env, objClass, "nativePtr", "J"), ptr);
        });
    }

    template<template<class> class Filter>
    void destroy( JNIEnv* env, jobject obj ) {
        jni_call(env, [&] {
            jclass objClass = env->GetObjectClass(obj);
            jboolean isInteger = env->GetBooleanField(obj, safe_GetFieldID(env, objClass, "isInteger", "Z"));
            jlong nativePtr = env->GetLongField(obj, safe_GetFieldID(env, objClass, "nativePtr", "J"));
            if (isInteger)
                delete (Filter<U8>*)nativePtr;
            else
                delete (Filter<F32>*)nativePtr;
        });
    }
}

extern "C" {
// Global Otsu
JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeGlobalOtsu_nativeinit(
        JNIEnv* env, jobject obj, jdouble minValue, jdouble maxValue, jdouble scale, jboolean down ) {
    create(env, obj, [&]( auto pixel ) {
        return new fast::GlobalOtsuBinaryFilter<decltype(pixel)>(minValue, maxValue, (bool)down, (double)scale);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeGlobalOtsu_nativedestroy( JNIEnv* env, jobject obj ) {
    destroy<fast::GlobalOtsuBinaryFilter>(env, obj);
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeGlobalOtsu_nativeprocess(
        JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<fast::GlobalOtsuBinaryFilter>(env, obj, jinput, joutput);
}

// Local mean
JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeLocalMean_nativeinit(
        JNIEnv* env, jobject obj, jobject jregionWidth, jdouble scale, jboolean down ) {
    create(env, obj, [&]( auto pixel ) {
        return new fast::LocalMeanBinaryFilter<decltype(pixel)>(
                config_from_java<ConfigLength>(env, jregionWidth), (float)scale, (bool)down);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeLocalMean_nativedestroy( JNIEnv* env, jobject obj ) {
    destroy<fast::LocalMeanBinaryFilter>(env, obj);
}

JNIEXPORT void JNICALL Java_org_boofcpp_fast_threshold_NativeLocalMean_nativeprocess(
        JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<fast::LocalMeanBinaryFilter>(env, obj, jinput, joutput);
}
}
