#ifndef BOOFCPP_THRESHOLDJNI_H
#define BOOFCPP_THRESHOLDJNI_H

#include <jni.h>
#include "JNIBoofCPP.h"

/// Thresholds input into output with the filter
template<template<class> class Filter, class E>
void threshold_images( Filter<E>* filter, JNIEnv* env, jobject jinput, jobject joutput ) {
    ImageAndInfo<boofcv::Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
    ImageAndInfo<boofcv::Gray<boofcv::U8>, JImageInfo> output = wrapCriticalGray<boofcv::U8>(env, joutput);
    CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
    CriticalArray output_data(env, output.info.jdata, 0);
    input.image.data = (E*)input_data.data;
    output.image.data = (boofcv::U8*)output_data.data;
    filter->process(input.image, output.image);
}

/// Thresholds a U8 or F32 input image into a U8 binary image, using the native filter of a Java NativeThresholdBase.
/// Filter<E> is the filter's class for pixel type E. C++ exceptions become Java exceptions
template<template<class> class Filter>
void threshold_process( JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jboolean isInteger = env->GetBooleanField(obj, safe_GetFieldID(env, objClass, "isInteger", "Z"));
        jlong nativePtr = env->GetLongField(obj, safe_GetFieldID(env, objClass, "nativePtr", "J"));
        if (isInteger)
            threshold_images((Filter<boofcv::U8>*)nativePtr, env, jinput, joutput);
        else
            threshold_images((Filter<boofcv::F32>*)nativePtr, env, jinput, joutput);
    });
}

#endif
