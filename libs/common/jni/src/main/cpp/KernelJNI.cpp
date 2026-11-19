#include "KernelJNI.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

// make sure you free the kernel afterwards!
boofcv::Kernel1D<S32>* extractKernel1D_S32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = safe_GetFieldID(env, objClass, "data", "[I");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jint* data = env->GetIntArrayElements((jintArray)jarray, 0);
    jint length = env->GetArrayLength((jintArray)jarray);

    // copy the Java array into the kernel
    auto kernel1D = new boofcv::Kernel1D<S32>((uint32_t)width,(uint32_t)offset);
    kernel1D->data.set((S32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseIntArrayElements((jintArray)jarray, data, 0);

    // return the results
    return kernel1D;
}

boofcv::Kernel1D<F32>* extractKernel1D_F32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = safe_GetFieldID(env, objClass, "data", "[F");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jfloat* data = env->GetFloatArrayElements((jfloatArray)jarray, 0);
    jint length = env->GetArrayLength((jfloatArray)jarray);

    // copy the Java array into the kernel
    auto kernel1D = new boofcv::Kernel1D<F32>((uint32_t)width,(uint32_t)offset);
    kernel1D->data.set((F32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseFloatArrayElements((jfloatArray)jarray, data, 0);

    // return the results
    return kernel1D;
}

boofcv::Kernel2D<boofcv::S32>* extractKernel2D_S32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = safe_GetFieldID(env, objClass, "data", "[I");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jint* data = env->GetIntArrayElements((jintArray)jarray, 0);
    jint length = env->GetArrayLength((jintArray)jarray);

    // copy the Java array into the kernel
    auto kernel2D = new boofcv::Kernel2D<S32>((uint32_t)width,(uint32_t)offset);
    kernel2D->data.set((S32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseIntArrayElements((jintArray)jarray, data, 0);

    // return the results
    return kernel2D;
}

boofcv::Kernel2D<boofcv::F32>* extractKernel2D_F32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = safe_GetFieldID(env, objClass, "data", "[F");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jfloat* data = env->GetFloatArrayElements((jfloatArray)jarray, 0);
    jint length = env->GetArrayLength((jfloatArray)jarray);

    // copy the Java array into the kernel
    auto kernel2D = new boofcv::Kernel2D<F32>((uint32_t)width,(uint32_t)offset);
    kernel2D->data.set((F32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseFloatArrayElements((jfloatArray)jarray, data, 0);

    // return the results
    return kernel2D;
}
