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
        ImageAndInfo<Gray<U8>,JImageCritical> input = wrapCriticalGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageCritical> output = wrapCriticalGrayU8(env,joutput);
        ImageAndInfo<Gray<U8>,JImageCritical> storage = wrapCriticalGrayU8(env,jstorage);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)storage.info.jdata, storage.info.data, 0);
    } else {
        ImageAndInfo<Gray<F32>,JImageCritical> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageCritical> output = wrapCriticalGrayF32(env,joutput);
        ImageAndInfo<Gray<F32>,JImageCritical> storage = wrapCriticalGrayF32(env,jstorage);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.info.data, 0);
        env->ReleasePrimitiveArrayCritical((jarray)storage.info.jdata, storage.info.data, 0);
    }
}

}