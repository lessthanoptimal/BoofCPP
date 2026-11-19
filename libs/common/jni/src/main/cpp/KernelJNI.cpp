#include "KernelJNI.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// Field IDs of a kernel class, looked up once. The global reference keeps the class loaded, and so the IDs valid
    struct KernelClass {
        jclass cls;
        jfieldID width, offset, data;

        KernelClass( JNIEnv* env, const char* name, const char* data_type ) {
            jclass local = safe_FindClass(env, name);
            width = safe_GetFieldID(env, local, "width", "I");
            offset = safe_GetFieldID(env, local, "offset", "I");
            data = safe_GetFieldID(env, local, "data", data_type);
            cls = (jclass)env->NewGlobalRef(local);
            env->DeleteLocalRef(local);
        }
    };

    /// Copies a Java kernel of class 'info'. Throws if it's another class
    template<class Kernel, class JArray, class Copy>
    Kernel* extract( JNIEnv* env, jobject jkernel, const KernelClass& info, Copy copy ) {
        if (!env->IsInstanceOf(jkernel, info.cls))
            throw std::runtime_error("Unexpected kernel type");
        jint width = env->GetIntField(jkernel, info.width);
        jint offset = env->GetIntField(jkernel, info.offset);
        auto jarray = (JArray)env->GetObjectField(jkernel, info.data);
        jint length = env->GetArrayLength(jarray);

        auto kernel = new Kernel((uint32_t)width, (uint32_t)offset);
        kernel->data.resize((uint32_t)length);
        copy(jarray, length, kernel->data.data);
        return kernel;
    }

    auto copy_int = []( JNIEnv* env ) {
        return [env]( jintArray array, jint length, S32* dst ) { env->GetIntArrayRegion(array, 0, length, dst); };
    };
    auto copy_float = []( JNIEnv* env ) {
        return [env]( jfloatArray array, jint length, F32* dst ) { env->GetFloatArrayRegion(array, 0, length, dst); };
    };
}

boofcv::Kernel1D<S32>* extractKernel1D_S32( JNIEnv *env, jobject& jkernel ) {
    static const KernelClass info(env, "boofcv/struct/convolve/Kernel1D_S32", "[I");
    return extract<Kernel1D<S32>, jintArray>(env, jkernel, info, copy_int(env));
}

boofcv::Kernel1D<F32>* extractKernel1D_F32( JNIEnv *env, jobject& jkernel ) {
    static const KernelClass info(env, "boofcv/struct/convolve/Kernel1D_F32", "[F");
    return extract<Kernel1D<F32>, jfloatArray>(env, jkernel, info, copy_float(env));
}

boofcv::Kernel2D<boofcv::S32>* extractKernel2D_S32( JNIEnv *env, jobject& jkernel ) {
    static const KernelClass info(env, "boofcv/struct/convolve/Kernel2D_S32", "[I");
    return extract<Kernel2D<S32>, jintArray>(env, jkernel, info, copy_int(env));
}

boofcv::Kernel2D<boofcv::F32>* extractKernel2D_F32( JNIEnv *env, jobject& jkernel ) {
    static const KernelClass info(env, "boofcv/struct/convolve/Kernel2D_F32", "[F");
    return extract<Kernel2D<F32>, jfloatArray>(env, jkernel, info, copy_float(env));
}
