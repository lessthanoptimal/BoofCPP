#include <jni.h>
#include <binary_ops.h>
#include "JNIBoofCPP.h"

using namespace boofcv;

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativeinit(
    JNIEnv *env, jobject obj, jdouble minValue, jdouble maxValue, jboolean down ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    jlong ptr;
    if( isInteger ) {
        ptr = (jlong)new GlobalOtsuBinaryFilter<U8>((U8)minValue,(U8)maxValue, (bool) down);
    } else {
        ptr = (jlong)new GlobalOtsuBinaryFilter<F32>((F32)minValue,(F32)maxValue, (bool) down);
    }
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    if( isInteger ) {
        delete (GlobalOtsuBinaryFilter<U8> *) nativePtr;
    } else {
        delete (GlobalOtsuBinaryFilter<F32> *) nativePtr;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalOtsu_nativeprocess
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {

    // Get the pointer to the thresholding algorithm
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    fid = env->GetFieldID(objClass, "nativePtr", "J");
    if( env->ExceptionCheck() )
        return;
    jlong nativePtr = env->GetLongField(obj, fid);
    if( env->ExceptionCheck() )
            return;

    JImageInfoU8 outputInfo = extractInfoU8(env,joutput);

    Gray<U8> output((U8*)outputInfo.data,(uint32_t)outputInfo.dataLength,
                   (uint32_t)outputInfo.width,(uint32_t)outputInfo.height,
                   (uint32_t)outputInfo.offset,(uint32_t)outputInfo.stride);

    try {
//        printf("   alg.threshold %d\n",alg->threshold);
        if( isInteger ) {
            ImageAndInfo<Gray<U8>,JImageInfoU8> input = wrapGrayU8(env,jinput);
            ((GlobalOtsuBinaryFilter<U8>*)nativePtr)->process(input.image, output);
            env->ReleaseByteArrayElements((jbyteArray)input.info.jdata, input.info.data, 0);
        } else {
            ImageAndInfo<Gray<F32>,JImageInfoF32> input = wrapGrayF32(env,jinput);
            ((GlobalOtsuBinaryFilter<F32> *)nativePtr)->process(input.image, output);
            env->ReleaseFloatArrayElements((jfloatArray)input.info.jdata, input.info.data, 0);
        }
    } catch( ... ) {
        printf("Exception!!\n");
        // TODO do something here
    }

    // Release the arrays
    env->ReleaseByteArrayElements((jbyteArray)outputInfo.jdata, outputInfo.data, 0);
}

}