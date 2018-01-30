#include <jni.h>
#include <binary_ops.h>

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_nativeinit(
    JNIEnv *env, jobject obj, jdouble threshold, jboolean down) {

    jclass objClass = env->GetObjectClass(obj);

    GlobalFixedBinaryFilter *alg = new GlobalFixedBinaryFilter((U8)threshold,(bool)down);

    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "L");
    env->SetIntField(obj, fid, (jlong)alg);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_nativedestroy(JNIEnv *env, jobject obj) {

    jclass configClass = env->GetObjectClass(configuration);
    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "L");
    jlong nativePtr = env->GetIntField(obj, fid);

    delete (GlobalFixedBinaryFilter *)nativePtr;
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_process
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {

}

}