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
        ImageAndInfo<Gray<U8>,JImageInfoU8> input = wrapGrayU8(env,jinput);
        ImageAndInfo<Gray<U8>,JImageInfoU8> output = wrapGrayU8(env,joutput);
        ImageAndInfo<Gray<U8>,JImageInfoU8> storage = wrapGrayU8(env,jstorage);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleaseByteArrayElements((jbyteArray)input.info.jdata, input.info.data, 0);
        env->ReleaseByteArrayElements((jbyteArray)output.info.jdata, output.info.data, 0);
        env->ReleaseByteArrayElements((jbyteArray)storage.info.jdata, storage.info.data, 0);
    } else {
        ImageAndInfo<Gray<F32>,JImageInfoF32> input = wrapGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfoF32> output = wrapGrayF32(env,joutput);
        ImageAndInfo<Gray<F32>,JImageInfoF32> storage = wrapGrayF32(env,jstorage);

        BlurImageOps::mean(input.image,output.image,(uint32_t)radius,storage.image);

        env->ReleaseFloatArrayElements((jfloatArray)input.info.jdata, input.info.data, 0);
        env->ReleaseFloatArrayElements((jfloatArray)output.info.jdata, output.info.data, 0);
        env->ReleaseFloatArrayElements((jfloatArray)storage.info.jdata, storage.info.data, 0);
    }
}

}