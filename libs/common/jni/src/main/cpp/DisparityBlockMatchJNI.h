#ifndef BOOFCPP_DISPARITY_BLOCK_MATCH_JNI_H
#define BOOFCPP_DISPARITY_BLOCK_MATCH_JNI_H

/// The JNI code of a variant's NativeDisparityBlockMatch, for the variant's FactoryStereoDisparity and
/// FactoryStereoDisparity_MT. Include the variant's stereo disparity headers first. The variant's JNI file defines the
/// JNI functions with BOOFCPP_DISPARITY_BLOCK_MATCH_JNI

#include <jni.h>
#include <memory>
#include <stdexcept>
#include "JNIBoofCPP.h"
#include "ConfigDisparityJNI.h"
#include "boofcv/concurrency.h"

namespace disparity_jni {
    using namespace boofcv;

    /// The algorithm for whichever input and disparity type was configured. Only one is set
    struct BlockMatch {
        std::unique_ptr<StereoDisparityBlockMatch<U8, U8>> u8_pixel;
        std::unique_ptr<StereoDisparityBlockMatch<U8, F32>> u8_subpixel;
        std::unique_ptr<StereoDisparityBlockMatch<F32, U8>> f32_pixel;
        std::unique_ptr<StereoDisparityBlockMatch<F32, F32>> f32_subpixel;
    };

    /// The single threaded algorithm, used with one thread, and the concurrent one, used with more
    struct NativeBlockMatch {
        BlockMatch single;
        BlockMatch concurrent;
    };

    template<class E>
    ImageAndInfo<Gray<E>, JImageInfo> wrap_input( JNIEnv* env, jobject jimage ) {
        if constexpr (std::is_same<E, U8>::value)
            return wrapCriticalGrayU8(env, jimage);
        else
            return wrapCriticalGrayF32(env, jimage);
    }

    template<class E, class D>
    void process( JNIEnv* env, StereoDisparityBlockMatch<E, D>& alg, jobject jleft, jobject jright,
                  ImageAndInfo<Gray<D>, JImageInfo>& disparity, jobject jscore ) {
        // Look up everything before acquiring critical arrays. No JNI calls are allowed while they're held
        ImageAndInfo<Gray<E>, JImageInfo> left = wrap_input<E>(env, jleft);
        ImageAndInfo<Gray<E>, JImageInfo> right = wrap_input<E>(env, jright);
        // Constructed, not assigned, because Gray's assignment would leak a default image's memory
        std::unique_ptr<ImageAndInfo<Gray<F32>, JImageInfo>> score;
        if (jscore != nullptr)
            score = std::make_unique<ImageAndInfo<Gray<F32>, JImageInfo>>(wrapCriticalGrayF32(env, jscore));

        CriticalArray left_data(env, left.info.jdata, JNI_ABORT);
        CriticalArray right_data(env, right.info.jdata, JNI_ABORT);
        CriticalArray disparity_data(env, disparity.info.jdata, 0);
        std::unique_ptr<CriticalArray> score_data;
        if (jscore != nullptr)
            score_data = std::make_unique<CriticalArray>(env, score->info.jdata, 0);

        left.image.data = (E*)left_data.data;
        right.image.data = (E*)right_data.data;
        disparity.image.data = (D*)disparity_data.data;
        if (score)
            score->image.data = (F32*)score_data->data;

        alg.process(left.image, right.image, disparity.image, score ? &score->image : nullptr);
    }

    /// Creates the algorithm for the configuration with 'Factory'
    template<class Factory>
    void create( const ConfigDisparityBM& config, jboolean image_f32, BlockMatch& alg ) {
        if (image_f32) {
            if (config.subpixel)
                alg.f32_subpixel = Factory::template block_match<F32, F32>(config);
            else
                alg.f32_pixel = Factory::template block_match<F32, U8>(config);
        } else {
            if (config.subpixel)
                alg.u8_subpixel = Factory::template block_match<U8, F32>(config);
            else
                alg.u8_pixel = Factory::template block_match<U8, U8>(config);
        }
    }

    /// Creates the single threaded algorithm with 'Factory' and the concurrent one with 'Factory_MT'
    template<class Factory, class Factory_MT>
    NativeBlockMatch* create( JNIEnv* env, jobject jconfig, jboolean image_f32 ) {
        ConfigDisparityBM config = config_from_java<ConfigDisparityBM>(env, jconfig);
        auto alg = std::make_unique<NativeBlockMatch>();
        create<Factory>(config, image_f32, alg->single);
        create<Factory_MT>(config, image_f32, alg->concurrent);
        return alg.release();
    }

    inline void process_any( JNIEnv* env, NativeBlockMatch* native, jobject jleft, jobject jright, jobject jdisparity,
                             jobject jscore ) {
        if (native == nullptr)
            throw std::runtime_error("Native algorithm has been destroyed");
        BlockMatch* alg = BoofConcurrency::max_threads() > 1 ? &native->concurrent : &native->single;
        if (alg->u8_subpixel || alg->f32_subpixel) {
            ImageAndInfo<Gray<F32>, JImageInfo> disparity = wrapCriticalGrayF32(env, jdisparity);
            if (alg->u8_subpixel)
                process(env, *alg->u8_subpixel, jleft, jright, disparity, jscore);
            else
                process(env, *alg->f32_subpixel, jleft, jright, disparity, jscore);
        } else {
            ImageAndInfo<Gray<U8>, JImageInfo> disparity = wrapCriticalGrayU8(env, jdisparity);
            if (alg->u8_pixel)
                process(env, *alg->u8_pixel, jleft, jright, disparity, jscore);
            else
                process(env, *alg->f32_pixel, jleft, jright, disparity, jscore);
        }
    }
}

/// The JNI functions of NativeDisparityBlockMatch. PREFIX is the Java class's JNI prefix, e.g.
/// Java_org_boofcpp_disparity_NativeDisparityBlockMatch, FACTORY the variant's FactoryStereoDisparity and FACTORY_MT
/// its FactoryStereoDisparity_MT. C++ exceptions become Java exceptions
#define BOOFCPP_DISPARITY_BLOCK_MATCH_JNI(PREFIX, FACTORY, FACTORY_MT) \
JNIEXPORT void JNICALL PREFIX##_nativeinit( JNIEnv* env, jobject obj, jobject jconfig, jboolean image_f32 ) { \
    try { \
        disparity_jni::NativeBlockMatch* alg = disparity_jni::create<FACTORY, FACTORY_MT>(env, jconfig, image_f32); \
        static const jfieldID native_ptr = class_field_id(env, obj, "nativePtr", "J"); \
        env->SetLongField(obj, native_ptr, (jlong)alg); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
} \
JNIEXPORT void JNICALL PREFIX##_nativedestroy( JNIEnv* env, jobject obj ) { \
    try { \
        static const jfieldID native_ptr = class_field_id(env, obj, "nativePtr", "J"); \
        delete (disparity_jni::NativeBlockMatch*)env->GetLongField(obj, native_ptr); \
        env->SetLongField(obj, native_ptr, 0); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
} \
JNIEXPORT void JNICALL PREFIX##_nativeprocess( JNIEnv* env, jobject obj, jobject jleft, jobject jright, \
                                              jobject jdisparity, jobject jscore ) { \
    try { \
        static const jfieldID native_ptr = class_field_id(env, obj, "nativePtr", "J"); \
        disparity_jni::process_any(env, (disparity_jni::NativeBlockMatch*)env->GetLongField(obj, native_ptr), jleft, \
                                   jright, jdisparity, jscore); \
    } catch (...) { \
        throw_java_exception(env); \
    } \
}

#endif
