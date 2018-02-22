#include <jni.h>
#include <image_blur.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeImageBlurOps_nativeMean(
    JNIEnv *env, jobject obj, jobject jinput, jobject joutput, jint radius, jobject jstorage) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);

    if( isInteger ) {
        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfo> output = wrapCriticalGrayU8(env,joutput);
        ImageAndInfo<Gray<U8>,JImageInfo> storage = wrapCriticalGrayU8(env,jstorage);

        input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);
        storage.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)storage.info.jdata, 0);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)storage.info.jdata, storage.image.data, 0);
    } else {
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        ImageAndInfo<Gray<F32>,JImageInfo> storage = wrapCriticalGrayF32(env,jstorage);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);
        storage.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)storage.info.jdata, 0);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)storage.info.jdata, storage.image.data, 0);
    }
}

}