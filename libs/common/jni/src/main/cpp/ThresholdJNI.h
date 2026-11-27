#ifndef BOOFCPP_THRESHOLDJNI_H
#define BOOFCPP_THRESHOLDJNI_H

#include <jni.h>
#include <type_traits>
#include "boofcv/concurrency.h"
#include "JNIBoofCPP.h"

/// Thresholds input into output with process(filter, input, output)
template<template<class> class Filter, class E, class Process>
void threshold_images( Filter<E>* filter, JNIEnv* env, jobject jinput, jobject joutput, Process process ) {
    ImageAndInfo<boofcv::Gray<E>, JImageInfo> input = wrapCriticalGray<E>(env, jinput);
    ImageAndInfo<boofcv::Gray<boofcv::U8>, JImageInfo> output = wrapCriticalGray<boofcv::U8>(env, joutput);
    CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
    CriticalArray output_data(env, output.info.jdata, 0);
    input.image.data = (E*)input_data.data;
    output.image.data = (boofcv::U8*)output_data.data;
    process(*filter, input.image, output.image);
}

/// Thresholds a U8 or F32 input image into a U8 binary image, using the native filter of a Java NativeThresholdBase.
/// Filter<E> is the filter's class for pixel type E, and process(filter, input, output) applies it. C++ exceptions
/// become Java exceptions
template<template<class> class Filter, class Process>
void threshold_process( JNIEnv* env, jobject obj, jobject jinput, jobject joutput, Process process ) {
    jni_call(env, [&] {
        const NativeBaseFields& fields = NativeBaseFields::get(env);
        jboolean isInteger = env->GetBooleanField(obj, fields.isInteger);
        jlong nativePtr = env->GetLongField(obj, fields.nativePtr);
        if (isInteger)
            threshold_images<Filter, boofcv::U8>((Filter<boofcv::U8>*)nativePtr, env, jinput, joutput, process);
        else
            threshold_images<Filter, boofcv::F32>((Filter<boofcv::F32>*)nativePtr, env, jinput, joutput, process);
    });
}

/// threshold_process() with Filter's process_with<Concurrent>() when BoofConcurrency::max_threads() > 1, otherwise
/// its single threaded process()
template<template<class> class Filter, class Concurrent>
void threshold_process_with( JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<Filter>(env, obj, jinput, joutput, []( auto& filter, const auto& input, auto& output ) {
        if (boofcv::BoofConcurrency::max_threads() > 1)
            filter.template process_with<Concurrent>(input, output);
        else
            filter.process(input, output);
    });
}

/// threshold_process() with ThresholdBlock_MT's process() when BoofConcurrency::max_threads() > 1, otherwise that of
/// its single threaded filter
template<template<class> class Filter>
void threshold_process_block( JNIEnv* env, jobject obj, jobject jinput, jobject joutput ) {
    threshold_process<Filter>(env, obj, jinput, joutput, []( auto& filter, const auto& input, auto& output ) {
        using Single = typename std::decay_t<decltype(filter)>::single_threaded;
        if (boofcv::BoofConcurrency::max_threads() > 1)
            filter.process(input, output);
        else
            filter.Single::process(input, output);
    });
}

#endif
