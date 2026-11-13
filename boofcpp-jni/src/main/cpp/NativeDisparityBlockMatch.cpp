#include <jni.h>
#include <memory>
#include <stereo_disparity.h>
#include "JNIBoofCPP.h"
#include "ConfigDisparityJNI.h"

using namespace boofcv;

namespace {
    /// The algorithm for whichever input and disparity type was configured. Only one is set
    struct NativeBlockMatch {
        std::unique_ptr<StereoDisparityBlockMatch<U8, U8>> u8_pixel;
        std::unique_ptr<StereoDisparityBlockMatch<U8, F32>> u8_subpixel;
        std::unique_ptr<StereoDisparityBlockMatch<F32, U8>> f32_pixel;
        std::unique_ptr<StereoDisparityBlockMatch<F32, F32>> f32_subpixel;
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
}

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_disparity_NativeDisparityBlockMatch_nativeinit(
        JNIEnv* env, jobject obj, jobject jconfig, jboolean image_f32 ) {
    try {
        ConfigDisparityBM config = config_from_java<ConfigDisparityBM>(env, jconfig);
        auto alg = std::make_unique<NativeBlockMatch>();
        if (image_f32) {
            if (config.subpixel)
                alg->f32_subpixel = FactoryStereoDisparity::block_match<F32, F32>(config);
            else
                alg->f32_pixel = FactoryStereoDisparity::block_match<F32, U8>(config);
        } else {
            if (config.subpixel)
                alg->u8_subpixel = FactoryStereoDisparity::block_match<U8, F32>(config);
            else
                alg->u8_pixel = FactoryStereoDisparity::block_match<U8, U8>(config);
        }
        jclass cls = env->GetObjectClass(obj);
        env->SetLongField(obj, safe_GetFieldID(env, cls, "nativePtr", "J"), (jlong)alg.release());
    } catch (...) {
        throw_java_exception(env);
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_disparity_NativeDisparityBlockMatch_nativedestroy( JNIEnv* env, jobject obj ) {
    try {
        delete get_native_ptr<NativeBlockMatch>(env, obj);
        jclass cls = env->GetObjectClass(obj);
        env->SetLongField(obj, safe_GetFieldID(env, cls, "nativePtr", "J"), 0);
    } catch (...) {
        throw_java_exception(env);
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_disparity_NativeDisparityBlockMatch_nativeprocess(
        JNIEnv* env, jobject obj, jobject jleft, jobject jright, jobject jdisparity, jobject jscore ) {
    try {
        NativeBlockMatch* alg = get_native_ptr<NativeBlockMatch>(env, obj);
        if (alg == nullptr)
            throw std::runtime_error("Native algorithm has been destroyed");
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
    } catch (...) {
        throw_java_exception(env);
    }
}
}
